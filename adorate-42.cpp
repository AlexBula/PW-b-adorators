#include "blimit.hpp"
#include "log.h"
#include <iostream>
#include <string>
#include <fstream>
#include <set>
#include <regex>
#include <map>
#include <atomic>
#include <mutex>
#include <memory>
#include <deque>
#include <thread>



const std::string REGEX = "([0-9]+) ([0-9]+) ([0-9]+)";

std::vector<int> convert;
std::map<int,int> new_numbers;

std::vector<int> b;
// std::deque<std::atomic<int>> b;
std::deque<std::atomic<int>> db;

std::vector<int> V;
std::vector<int> R;

// Mapa krawedz -> waga
std::map<std::pair<int,int>,int> W; 

// Set wszystkich wierzchołkow
std::set<int> NODES;


int numer = 0;

struct paircmp {
    bool operator() (std::pair<int,int> const & lhs, std::pair<int,int> const & rhs) const
    {
    return (lhs.second > rhs.second) || (lhs.second == rhs.second && convert[lhs.first] > convert[rhs.first]);
    }
};

std::vector<std::set<std::pair<int,int>,paircmp>> N;
std::vector<std::set<std::pair<int,int>,paircmp>> S;


static std::regex graf(REGEX);

std::atomic_flag argmax_lock = ATOMIC_FLAG_INIT;

std::vector<std::unique_ptr<std::mutex>> SMutexes;

std::mutex RMutex;

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


void update_bvalue(int b_method) {

    // b.clear();
    for (auto i : NODES) {
        // b.emplace_back(bvalue(b_method, i));
        b[new_numbers[i]] = bvalue(b_method, i);
    }
}

void zero_db() {
    db.clear();
    for (auto v : NODES) {
        db.emplace_back(0);
    }
}


void clear_S() {
    for (auto it = S.begin(); it != S.end(); ++it) {
        it->clear();
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
        if (S[v].size() < bvalue((uint)b_method, (ulong)convert[v])) {
            return v;
        }
        else if (bvalue(b_method, (ulong)convert[v]) > 0) {
            std::pair<int,int> p2 = *(--S[v].end());
            // p2.first = convertp2.first;
            if (compare(p, p2)) return v;
        }
    }
    return -1;
}

bool is_eligible(int x, int u, int b_method) {
    struct paircmp compare;
    if (S[x].find({u, W[{u,x}]}) != S[x].end()) return false;
    if (S[x].size() < bvalue((uint)b_method, (ulong)convert[x]) 
        || (bvalue(b_method, (ulong)convert[x]) > 0) && compare({u,W[{u,x}]}, *(--S[x].end()))) {
            return true;
        }
        else return false;
}
void get_adorators(int u, int b_method, int id) {

    int i = 0;
    int y;
    while (i < b[u]) {
        // wybieram argmax
        int x = argmax(u, b_method);

        if (x == -1) return;
        else {
            SMutexes[x]->lock();

            if (is_eligible(x, u, b_method)) {

                if (S[x].size() < bvalue((uint)b_method, (ulong)convert[x])) y = -1;
                else {
                    auto p = *(--S[x].end());
                    y = p.first;
                }
                // log("get_adorators: watek ", id, ", wybral wezel ", convert[x]);
                S[x].insert({u,W[{x,u}]});
                if (y != -1) {
                    RMutex.lock();
                    S[x].erase(--S[x].end());
                    // log("get_adorators: watek ", id,  ", zwieksza wezel  ", convert[y]);
                    if (db[y] == 0) {
                        R.push_back(y);
                    }
                    db[y]++;
                    RMutex.unlock();
                }
                i++;
            }
            SMutexes[x]->unlock();
        }

    }
}

void add_new_node(int v) {
    new_numbers[v] = numer;
    b.push_back(0);
    db.emplace_back(0);   
    N.push_back(std::set<std::pair<int,int>,paircmp>());
    S.push_back(std::set<std::pair<int,int>,paircmp>());
    convert.push_back(v);
    NODES.insert(v);
    SMutexes.push_back(std::make_unique<std::mutex>());
    numer++;
}


void copy_db_to_b() {
    for (int i = 0; i < db.size(); ++i) {
        b[i] = db[i];
    }
}

void find_adorators(std::vector<int> C, int b_method, int id) {

    for (auto it = C.begin(); it != C.end(); ++it) {
        get_adorators(*it, b_method, id);
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

    if (file.is_open()) {
        while (getline(file, line)) {
            if (line[0] != '#' && !line.empty() && std::regex_match(line, matches, graf)) {
    
                x = std::stoi(matches[1].str());
                y = std::stoi(matches[2].str());
                w = std::stoi(matches[3].str());
                
                if (NODES.find(x) == NODES.end()) add_new_node(x);
                if (NODES.find(y) == NODES.end()) add_new_node(y);

                int nx = new_numbers[x];
                int ny = new_numbers[y];
                W.insert(std::make_pair(std::make_pair(nx,ny),w));
                W.insert(std::make_pair(std::make_pair(ny,nx),w));
                N[nx].insert(std::make_pair(ny,w));
                N[ny].insert(std::make_pair(nx,w));

            }
        }
    } else {
        std::cerr << "Error: couldn't open file: " << input_filename << '\n';
    } 


    int l = 0;
    for (auto i : convert) {
        V.push_back(l);
        l++;
    }


    std::vector<int> Q;
    for (int b_method = 0; b_method < b_limit + 1  ; b_method++) {
        Q = V;
        update_bvalue(b_method);
        while (!Q.empty()) {

            std::thread threads[thread_count];

            std::vector<int> C[thread_count];

            for (auto i = 0; i < Q.size(); ++i) {
                C[i % thread_count].push_back(Q[i]);
            }

            // Odpalanie puli watkow;
            for (auto i = 0; i < thread_count; ++i) {
                threads[i] = std::thread(find_adorators, C[i], b_method, i);
            }

            for (auto i = 0; i < thread_count; ++i) {
                threads[i].join();
            }

            Q = R;
            R.clear();
            copy_db_to_b();
            zero_db();
        }


        // wypisz_adorowanych();
        long long sum = 0;
        
        for (auto it = 0; it < S.size(); ++it) {
            std::set<std::pair<int,int>, paircmp> se = S[it];
            for (auto its = se.begin(); its != se.end(); ++its) {
                // std::cout << "Wezeł " << convert[it] << " jest adorowany przez " << convert[its->first] << '\n';
                sum += its->second;
            }
        }
        std::cout << sum/2 << std::endl;
        clear_S();
    }
    


}
