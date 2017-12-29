#include "blimit.hpp"
#include <iostream>
#include <string>
#include <unordered_map>
#include <list>
#include <fstream>
#include <unordered_set>
#include <set>
#include <cassert>
#include <regex>
#include <map>
#include <atomic>
#include <mutex>

const std::string REGEX = "([0-9]+) ([0-9]+) ([0-9]+)";


struct pairhash {
public:
  template <typename T, typename U>
  std::size_t operator()(const std::pair<T, U> &x) const
  {
    return std::hash<T>()(x.first) ^ std::hash<U>()(x.second);
  }
};


struct paircmp {
    bool operator() (std::pair<int,int> const & lhs, std::pair<int,int> const & rhs) const
    {
    return (lhs.second > rhs.second) || (lhs.second == rhs.second && lhs.first > rhs.first);
    }
};


struct cmp {
    bool operator() (int const & lhs, int const & rhs) const
    {
    return lhs > rhs;
    }
};


// Mapa wezeł -> sasiedzi wezla
std::map<int,std::set<std::pair<int,int>,paircmp>> N;

// Mapa krawedz (v,w) -> waga
std::map<std::pair<int,int>,int> W; 

// Set wszystkich wierzchołkow
std::set<int> NODES;

 // Mapa wierzcholek -> set par (wierzchołek, waga) które go adoruja
std::unordered_map<int,std::set<std::pair<int,int>,paircmp>> S;

std::map<int,int> b;
std::map<int,int> db;


std::vector<int> V;
std::vector<int> R;

static std::regex graf(REGEX);

std::atomic_flag argmax_lock = ATOMIC_FLAG_INIT;

std::map<int,std::mutex> mutexes;

void wypisz_adorowanych() {

    for (auto it = NODES.begin(); it != NODES.end(); ++it) {
        auto wezel = *it;

        std::cout << "Węzeł " << wezel  << " jest adorowany przez:\n";
        for (auto i = S[wezel].begin(); i != S[wezel].end(); ++i) {
            std::cout << "wezel " << i->first << ", waga krawedzi " << i->second << '\n';
        } 
        // std::cout << '\n';
    }
}


void T_597(int n) {
    std::cout << "Węzeł " << n << " jest adorowany przez:\n";
    for (auto i = S[n].begin(); i != S[n].end(); ++i) {
        std::cout << "wezel " << i->first << ", waga krawedzi " << i->second << '\n';
    } 
}




// Argmax (zwraca -1 zamiast nulla)
int argmax(int u, int b_method) {

    struct paircmp compare;

    // Przechodze po wszystkich wezlach az znajde taki ktory mi pasuje
    for (auto it = N[u].begin(); it != N[u].end(); ++it) {
        
        int v = it->first;
        auto p = std::make_pair(u, W[{u,v}]);

        if (S[v].find({u,W[{v,u}]}) != S[v].end()) continue;
        if (S[v].size() < bvalue((uint)b_method, (ulong)v)) {
            return v;
        }
        else if (bvalue(b_method, (ulong)v) > 0) {
            std::pair<int,int> p2 = *(--S[v].end());
            if (compare(p, p2)) return v;
        }
    }
    return -1;
}



void get_adorators(int u, int b_method) {

    int i = 0;
    int y;
    while (i < b[u]) {
        // wybieram argmax
        int x = argmax(u, b_method);

        if (x == -1) return;
        else {
            if (S[x].size() < bvalue((uint)b_method, (ulong)x)) y = -1;
            else {
                auto p = *(--S[x].end());
                y = p.first;
            }
            S[x].insert({u,W[{x,u}]});
            if (y != -1) {
                S[x].erase(--S[x].end());
                if (db[y] == 0) R.push_back(y);
                db[y]++;
            }
            i++;
        }

    }
}


void update_bvalue(int b_method) {

    for (auto i : NODES) {
        b[i] = bvalue(b_method, i);
    }
}

void zero_db() {
    for (auto v : NODES) {
        db[v] = 0;
    }
}

int main(int argc, char* argv[]) {
    std::ios_base::sync_with_stdio(false);

    if (argc != 4) {
        std::cerr << "usage: "<<argv[0]<<" thread-count inputfile b-limit"<< std::endl;
        return 1;
    }


    int thread_count = std::stoi(argv[1]);
    int b_limit = std::stoi(argv[3]);
    std::string input_filename{argv[2]};
    
    std::ifstream file(input_filename);
    std::string line;
    int x, y, w;
    std::smatch matches;

    int numer = 0;
    
    if (file.is_open()) {
        while (getline(file, line)) {
            if (line[0] != '#' && !line.empty() && std::regex_match(line, matches, graf)) {
                
                x = std::stoi(matches[1].str());
                y = std::stoi(matches[2].str());
                w = std::stoi(matches[3].str());
                W.insert(std::make_pair(std::make_pair(x,y),w));
                W.insert(std::make_pair(std::make_pair(y,x),w));
                N[x].insert(std::make_pair(y,w));
                N[y].insert(std::make_pair(x,w));
                if (NODES.find(x) == NODES.end()) {
                    NODES.insert(x);
                    db[x] = 0;
                    // mutexes.insert(std::make_pair(x,std::mutex()));
                }
                if (NODES.find(y) == NODES.end()) {
                    NODES.insert(y);
                    db[y] = 0;
                }
            }
        }

    } else {
        std::cerr << "Error: couldn't open file: " << input_filename << '\n';
    } 

    

    for (auto i : NODES) {
        V.push_back(i);
    }

    std::vector<int> Q;
    for (int b_method = 0; b_method < b_limit + 1; b_method++) {
        Q = V;
        update_bvalue(b_method);
        while (!Q.empty()) {

            for (auto it = Q.begin(); it != Q.end(); ++it) {
                // std::cout << "Rozpatruje wezel " << *it << '\n';
                get_adorators(*it, b_method);
            }
            Q = R;
            R.clear();
            b = db;
            zero_db();
        }


        // wypisz_adorowanych();
        long long sum = 0;
        
        for (auto it = S.begin(); it != S.end(); ++it) {
            std::set<std::pair<int,int>, paircmp> se = it->second;
            for (auto its = se.begin(); its != se.end(); ++its) {
                // std::cout << "Wezeł " << it->first << " jest adorowany przez " << its->first << '\n';
                sum += its->second;
            }
        }
        std::cout << sum/2 << std::endl;
        S.clear();
    }
    


}
