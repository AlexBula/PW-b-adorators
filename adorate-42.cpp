#include "blimit.hpp"
#include <stdio.h>
#include <fstream>
#include <set>
#include <regex>
#include <map>
#include <unordered_map>
#include <unordered_set>
#include <atomic>
#include <mutex>
#include <memory>
#include <deque>
#include <thread>
#include <chrono>
#include <sstream>


const std::string REGEX = "([0-9]+) ([0-9]+) ([0-9]+)";
static std::regex graf(REGEX);

std::vector<int> convert;
std::unordered_map<int,int> new_ids;

std::vector<int> b;
std::deque<std::atomic<int>> db;

std::map<std::pair<int,int>,int> W; 

std::vector<int> V;
std::vector<int> Q;
thread_local std::vector<int> R;
std::unordered_set<int> RS;
std::set<int> NODES;

int number = 0;

struct paircmp {
    inline bool operator() (std::pair<int,int> const & lhs, std::pair<int,int> const & rhs) const {
        return (lhs.second > rhs.second) || (lhs.second == rhs.second && convert[lhs.first] > convert[rhs.first]);
    }
};

static struct paircmp compare;

std::vector<std::vector<std::pair<int,int>>> N;
std::vector<std::set<std::pair<int,int>,paircmp>> S;

std::vector<std::unique_ptr<std::mutex>> SMutexes;
std::mutex QMutex;


void update_bvalues(int b_method) {
    for (auto i : NODES) {
        b[new_ids[i]] = bvalue(b_method, i);
    }
}

void zero_db() {
    std::fill(db.begin(), db.end(), 0);
}

void copy_db_to_b() {
    for (int i = 0; i < db.size(); ++i) {
        b[i] = db[i];
    }
}

int argmax(int u, int b_method) {

    for (auto it = N[u].begin(); it != N[u].end(); ++it) {
        SMutexes[it->first]->lock();
        if (S[it->first].find({u,W[{it->first,u}]}) != S[it->first].end()) {
            SMutexes[it->first]->unlock();
            continue;
        }
        if (S[it->first].size() < bvalue((uint)b_method, (ulong)convert[it->first])) {
            return it->first;
        }
        else if (bvalue(b_method, (ulong)convert[it->first]) > 0) {
            if (compare({u, W[{u,it->first}]}, *(--S[it->first].end()))) return it->first;
        }
        SMutexes[it->first]->unlock();
    }
    return -1;
}

bool is_eligible(int x, int u, int b_method) {

    if (S[x].find({u, W[{u,x}]}) != S[x].end()) return false;
    if (S[x].size() < bvalue((uint)b_method, (ulong)convert[x]) 
        || (bvalue(b_method, (ulong)convert[x]) > 0) && compare({u,W[{u,x}]}, *(--S[x].end()))) {
            return true;
        }
        else return false;
}
void get_adorators(int u, int b_method, int id) {

    int i = 0;
    while (i < b[u]) {

        int x = argmax(u, b_method);
        if (x == -1) return;
        else {
            // SMutexes[x]->lock();

            if (is_eligible(x, u, b_method)) {
                int y;
                if (S[x].size() < bvalue((uint)b_method, (ulong)convert[x])) y = -1;
                else {
                    y = (--S[x].end())->first;
                }
                S[x].insert({u,W[{x,u}]});
                if (y != -1) {
                    S[x].erase(--S[x].end());
                    if (db[y] == 0) {
                        R.push_back(y);
                    }
                    db[y]++;
                }
                ++i;
            }
            SMutexes[x]->unlock();
        }

    }
}

void add_new_node(int v) {
    new_ids[v] = number;
    b.push_back(0);
    db.emplace_back(0);   
    N.push_back(std::vector<std::pair<int,int>>());
    convert.push_back(v);
    NODES.insert(v);
    SMutexes.push_back(std::make_unique<std::mutex>());
    V.push_back(number);
    number++;
}



void find_adorators(std::vector<int> &C, int b_method, int id) {
    for (auto it = C.begin(); it != C.end(); ++it) {
        get_adorators(*it, b_method, id);
    }
    QMutex.lock();
    for (auto it = R.begin(); it != R.end(); ++it) RS.insert(*it);
    // RS.insert(RS.begin(), R.begin(), R.end());
    QMutex.unlock();
}

int main(int argc, char* argv[]) {
    auto start = std::chrono::high_resolution_clock::now();

    if (argc != 4) {
        fprintf(stderr, "usage %s thread-count inputfile b-limit\n", argv[0]);
        return 1;
    }


    int thread_count = std::stoi(argv[1]);
    int b_limit = std::stoi(argv[3]);
    char *input_filename{argv[2]};
    
    std::ifstream file(input_filename);
    std::string line;
    int x, y, w;
    std::smatch matches;

    if (file.is_open()) {
        while (getline(file, line)) {
            if (line[0] != '#' && !line.empty()) {// && std::regex_match(line, matches, graf)) {
                std::stringstream ss;
                // x = std::stoi(matches[1].str());
                // y = std::stoi(matches[2].str());
                // w = std::stoi(matches[3].str());
                ss << line;
                ss >> x >> y >> w;
                // printf("Numer to %d i %d i %d\n",x,y,w );
                if (NODES.find(x) == NODES.end()) add_new_node(x);
                if (NODES.find(y) == NODES.end()) add_new_node(y);

                int nx = new_ids[x];
                int ny = new_ids[y];
                W.insert(std::make_pair(std::make_pair(nx,ny),w));
                W.insert(std::make_pair(std::make_pair(ny,nx),w));
                N[nx].push_back({ny,w});
                N[ny].push_back({nx,w});

            }
        }
    } else {
        fprintf(stderr,"Error: couldn't open file:%s\n",input_filename);
    } 
    S = std::vector<std::set<std::pair<int,int>,paircmp>>(number);
    std::fill(S.begin(), S.end(), std::set<std::pair<int,int>,paircmp>());

    for (int it = 0; it < N.size(); ++it) {
        std::sort(N[it].begin(), N[it].end(), compare);
    }

    std::chrono::duration<double> elapsed(std::chrono::high_resolution_clock::now() - start);
    fprintf(stderr, "Graf wczytany po %f sekundach\n", elapsed.count());

    
    for (int b_method = 0; b_method < b_limit + 1  ; ++b_method) {
        start = std::chrono::high_resolution_clock::now();
        Q = V;
        update_bvalues(b_method);
        while (!Q.empty()) {

            std::thread threads[thread_count];
            std::vector<int> C[thread_count];

            for (auto i = 0; i < Q.size(); ++i) {
                C[i % thread_count].push_back(Q[i]);
            }

            Q.clear();
            for (auto i = 0; i < thread_count; ++i) {
                threads[i] = std::thread(find_adorators, std::ref(C[i]), b_method, i);
            }

            for (auto i = 0; i < thread_count; ++i) {
                threads[i].join();
            }

            for (auto it = RS.begin(); it != RS.end(); ++it) Q.push_back(*it);
            RS.clear();
            copy_db_to_b();
            zero_db();
        }

        int sum = 0;
        
        for (int it = 0; it < S.size(); ++it) {
            for (auto its = S[it].begin(); its != S[it].end(); ++its) {
                sum += its->second;
            }
            S[it].clear();
        }
        std::chrono::duration<double> elapsed = std::chrono::high_resolution_clock::now() - start;
        fprintf(stderr, "b_method %d time: %f\n",b_method, elapsed.count());
        printf("%d\n",sum / 2);
    }
}
