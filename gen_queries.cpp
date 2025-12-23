#include "common.hpp"

using namespace std;

void generate_queries(int N, int M, int Q, unsigned int seed) {
    std::mt19937 rng(seed);
    string f0= "./data/queries_0.txt";
    string f1 ="./data/queries_1.txt";
    std::ofstream f2("./data/queries_i.txt");
    std::ofstream f3("./data/queries_j.txt");
    for (int q = 0; q < Q; q++) {
        int i = abs(generate_int(rng))%M;                // same for both files
        int j = abs(generate_int(rng))%N;
        //cout<<j<<endl;
        pair<DPFKey, DPFKey> Keys= generateDPF(j,N,1);
        print_DPFKey_to_file(Keys.first,f0,q);
        print_DPFKey_to_file(Keys.second,f1,q);
        f2<<i<<"\n"<<std::flush;
        f3<<j<<"\n";
    }
    f2.close();
    f3.close();
}

int main(){
    int Q,N,M;
    Q= stoi(getenv("Q"));
    N= stoi(getenv("N"));
    M= stoi(getenv("M"));
    uint32_t seed = random_uint32();
    generate_queries(N,M,Q,seed);
}