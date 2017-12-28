#include "blimit.hpp"
#include <iostream>
#include <string>
#include <unordered_map>
#include <list>
#include <fstream>
#include <unordered_set>
#include <set>
#include <queue>
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
std::map<int,std::vector<int>> N;

// Mapa krawedz (v,w) -> waga
std::map<std::pair<int,int>,int> W; 

// Set wszystkich wierzchołkow
std::set<int> NODES;

 // Mapa wierzcholek -> set par (wierzchołek, waga) które go adoruja
std::unordered_map<int,std::set<std::pair<int,int>,paircmp>> S;

// Mapa wierzcholek -> set par (wierzcholek, waga) które sam adoruje 
std::unordered_map<int,std::set<int,cmp>> T; // 

// Mapa wierzcholek -> set par (wierzcholek, waga) ktorych jeszcze nie adoruje
// a sa moimi sasiadami
std::map<int,std::set<std::pair<int,int>,paircmp>> NT;
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

    std::cout << "Węzeł " << n << " adoruje:\n";
    for (auto i = T[n].begin(); i != T[n].end(); ++i) {
        std::cout << "wezel " << *i << '\n';// << ", waga krawedzi " << i->second << '\n';
    } 

}




// Argmax (zwraca -1 zamiast nulla)
int argmax(int u, int b_method) {

    // Jezeli nie mam juz nieadorowanych sasiadow zwracam -1
    if (NT[u].empty()) return -1;
    
    struct paircmp compare;

    // Przechodze po wszystkich wezlach az znajde taki ktory mi pasuje
    for (auto it = NT[u].begin(); it != NT[u].end(); ++it) {
        auto p = *it;
        int v = p.first;

        // Jezeli wezel nie przekroczyl limitu to go wybieram
        if (S[v].empty() || S[v].size() < bvalue((uint)b_method, (ulong)v)) {
            NT[u].erase(it);
            return v;
        }
        std::pair<int,int> p2 = *(--S[v].end());


        // Jezeli osiagnal limit sprawdzam czy jestem lepszy
        // albo wybieram albo zwracam -1
        if (S[v].size() == bvalue(b_method,v) && !compare(p, p2)) {
            continue;
        } else {
            NT[u].erase(it); // usuwam wezel z moich przyszlych kandydatow
            return v;
        }
    }
    return -1;
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

    if (file.is_open()) {
        while (getline(file, line)) {
            if (line[0] != '#' && !line.empty() && std::regex_match(line, matches, graf)) {
                
                x = std::stoi(matches[1].str());
                y = std::stoi(matches[2].str());
                w = std::stoi(matches[3].str());
                W.insert(std::make_pair(std::make_pair(x,y),w)); // dodaje krawedzi
                W.insert(std::make_pair(std::make_pair(y,x),w));
                if (NODES.find(x) == NODES.end()) {
                    NODES.insert(x); // dodaje wierzcholki do seta
                    // mutexes.insert(std::make_pair(x,std::mutex()));
                }
                if (NODES.find(y) == NODES.end()) NODES.insert(y);
                NT[x].insert({y,w}); // dodaje pary wierzcholek waga
                NT[y].insert({x,w});
            }
        }

    } else {
        std::cerr << "Error: couldn't open file: " << input_filename << '\n';
    } 

    
    std::vector<int> V;
    std::vector<int> R;

    for (auto i : NODES) {
        V.push_back(i);
    }

    for (int b_method = 0; b_method < b_limit + 1; b_method++) {
        std::vector<int> Q = V;
        while (!Q.empty()) {

            for (auto it = Q.begin(); it != Q.end(); ++it) {

                int u = *it;
                // std::cerr << "Rozpatruje node " << u << '\n';
                while (T[u].size() < bvalue((uint)b_method, (ulong)u)) {
                    // wybieram argmax
                    x = argmax(u, b_method);

                    if (x == -1) break;
                    else {

                        std::pair<int,int> p;
                        // jezeli nie wypelniony y = -1
                        if (S[x].size() < bvalue((uint)b_method, (ulong)x)) y = -1;
                        else {
                            // inaczej pobieram najgorszy wezel
                            // i go usuwam
                            p = *(--S[x].end());
                            y = p.first;
                            S[x].erase(--S[x].end());
                        }

                        // dodaje znalezionemu wezlowi
                        // ze jest przeze mnie adorowany, wraz z waga krawedzi
                        // nasepnie sam zapamietuje ze go adoruje
                        S[x].insert({u,W[{x,u}]});
                        T[u].insert(x);
                        // S[u].insert({x,W[{u,x}]});
                        // T[x].insert(u);
                        // jezeli pozbylem sie jakiegos wezla
                        if (y != -1) {

                            // mowie ze nie adoruje juz znalezionego wezla
                            T[y].erase(x);
                            // dodaje mu wezel x spowrotem do mozliwych adorowanych
                            // NT[y].insert({x,W[std::make_pair(y,x)]});
                            R.push_back(y);
                        }
                    }

                }
            }
            Q = R;
            R.clear();
        }
        // wypisz_adorowanych();
        long long sum = 0;
        
        // for (auto it = NODES.begin(); it != NODES.end(); ++it) {
        //     auto wezel = *it;
        //     for (auto its = T[wezel].begin(); its != T[wezel].end(); ++its) {
        //         sum += W[std::make_pair(wezel,*its)];
        //     }
        // }
        // std::cout << sum/2 << '\n';
        
        for (auto it = S.begin(); it != S.end(); ++it) {
            std::set<std::pair<int,int>, paircmp> se = it->second;
            for (auto its = se.begin(); its != se.end(); ++its) {
                std::cout << "Wezeł " << it->first << " jest połączony z " << its->first << '\n';
                sum += its->second;
            }
        }
        std::cout << sum/2 << '\n';
    }
    


}
