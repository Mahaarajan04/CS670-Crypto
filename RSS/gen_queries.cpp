#include "common.hpp"

using namespace std;

void generate_queries(int N, int M, int Q, unsigned int seed) {
    std::mt19937 rng(seed);
    std::ofstream f01("./data/queries_0_1.txt");
    std::ofstream f02("./data/queries_0_2.txt");
    std::ofstream f11("./data/queries_1_1.txt");
    std::ofstream f12("./data/queries_1_2.txt");
    std::ofstream f21("./data/queries_2_1.txt");
    std::ofstream f22("./data/queries_2_2.txt");
    std::ofstream f2("./data/queries_i.txt");
    std::ofstream f3("./data/queries_j.txt");
    for (int q = 0; q < Q; q++) {
        int i = abs(generate_int(rng))%M;                // same for both files
        int j = abs(generate_int(rng))%N;
        //cout<<j<<endl;
        for(int it=0;it<N;it++){
            int j0= abs(generate_int(rng))%(MOD);
            int j1=abs(generate_int(rng))%(MOD);
            int j2;
            if(it==j) j2= sub_mod(sub_mod(1,j0),j1);
            else j2= sub_mod(sub_mod(0,j0),j1);
            f01<< j0 << " ";
            f02<< j1 <<" ";
            f11<< j1 << " ";
            f12<< j2 <<" ";
            f21<< j2 << " ";
            f22<< j0 <<" ";
        }
        f01<< "\n"<<std::flush;
        f02<< "\n"<<std::flush;
        f11<< "\n"<<std::flush;
        f12<< "\n"<<std::flush;
        f21<< "\n"<<std::flush;
        f22<< "\n"<<std::flush;     
        f2<<i<<"\n"<<std::flush;
        f3<<j<<"\n"<<std::flush;
    }
    f01.close();
    f02.close();
    f11.close();
    f12.close();
    f21.close();
    f22.close();
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