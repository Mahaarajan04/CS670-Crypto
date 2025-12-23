#pragma once
#include <utility>
#include <boost/asio.hpp>
#include <boost/asio/awaitable.hpp>
#include <boost/asio/use_awaitable.hpp>
#include <iostream>
#include <fstream>
#include <random>
#include<vector>
#include <chrono>
#include<math.h>

#define MOD 101 //Alder Checksum so that mult of 2 numbers does

using boost::asio::awaitable;
using boost::asio::co_spawn;
using boost::asio::detached;
using boost::asio::use_awaitable;
using boost::asio::ip::tcp;
using std::vector;
using std::endl;
using std::cout;
using std::stoi;
using std::pair;
using std::string;

namespace this_coro = boost::asio::this_coro;

struct Msg {
    uint32_t seed1;
    uint32_t seed2;
};


int add_mod(int A, int B){
    long long C= (long long)A+B;
    return (int)(C>=MOD?C-MOD:C);
}

int sub_mod(int A, int B){
    long long C= (long long)A-B;
    return (int)(C>=0?C:MOD+C);
}

int mul_mod(int A, int B){
    long long C= (long long)A*B;
    C=C%MOD;
    return (int)C;
}

inline uint32_t random_uint32() {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<uint32_t> dis;
    return dis(gen);
}

vector<vector<int>> generate_matrix(int m, int n, std::mt19937& rng) {
    std::uniform_int_distribution<int> dist(0, MOD-1);
    vector<vector<int>> mat(m, vector<int>(n));
    for (int i = 0; i < m; i++) {
        for (int j = 0; j < n; j++) {
            mat[i][j] = dist(rng);
        }
    }
    return mat;
}

//------ Generation of a vector (added)---------
vector<int> generate_vector(int n, std::mt19937& rng) {
    std::uniform_int_distribution<int> dist(0, MOD-1);
    vector<int> vect(n);
    for (int i = 0; i < n; i++) {
        vect[i] = dist(rng);
    }
    return vect;
}

int generate_int(std::mt19937& rng){
    std::uniform_int_distribution<int> dist(0, MOD-1);
    return dist(rng);
} 

//------Read queries file-----//
vector<vector<int>> read_queries(int N, std::ifstream&f) {
    std::string line;
    vector<vector<int>>secret_targets;
    while (std::getline(f, line)) {
        std::istringstream iss(line);
        std::vector<int> row(N);
        for (int j = 0; j < N; j++) {
            if (!(iss >> row[j])) {
                throw std::runtime_error("Malformed line in ");
            }
        }
        secret_targets.push_back(std::move(row));
    }
    return secret_targets;
}

void print_matrix(const std::vector<std::vector<int>>& mat, int role, int flag, int flag1) {
    std::string filename;
    if(flag==0){
        filename = "./data/U_" + std::to_string(flag1)+"_" + std::to_string(role) + ".txt";
    }
    else{
        filename = "./data/V_" + std::to_string(flag1)+"_" + std::to_string(role) + ".txt";
    }
    std::ofstream f(filename);
    for (size_t i = 0; i < mat.size(); ++i) {
        for (size_t j = 0; j < mat[i].size(); ++j) {
             f<<mat[i][j]<<"   ";
        }
        f<<"\n";
    }
    f.close();
}

vector<int> add_vectors(const vector<int>A, const vector<int>B){
    vector<int>C;
    for(int i=0; i<A.size(); i++){
        C.push_back(add_mod(A[i],B[i]));
    }
    return C;
}

vector<int> sub_vectors(vector<int>&A, vector<int>&B){
    vector<int>C;
    for(int i=0; i<A.size(); i++){
        C.push_back(sub_mod(A[i],B[i]));
    }
    return C;
}

vector<int> col_extract(int k, vector<vector<int>>&mat){
    vector<int>ans;
    for(int i=0;i<mat.size();i++) ans.push_back(mat[i][k]);
    return ans;
}

vector<int> mult_vector_scalar(const vector<int>&A, int B){
    vector<int>C;
    for(int i=0; i<A.size(); i++){
        C.push_back(mul_mod(A[i],B));
    }
    return C;
}

void print_vector(const vector<int>A, std::ofstream &f){
    for(int i=0;i<A.size();i++) f<<A[i]<<"  ";
    f<<"\n";
}

int dot_vector(const vector<int>&A, const vector<int>&B){
    int ans=0;
    for(int i=0;i<A.size();i++) ans=add_mod(ans,mul_mod(A[i],B[i]));
    return ans;
}


inline std::vector<std::vector<int>> add_matrix(const std::vector<std::vector<int>>& A,
                                                const std::vector<std::vector<int>>& B) {
    int n = A.size(), m = A[0].size();
    std::vector<std::vector<int>> C(n, std::vector<int>(m));
    for (int i=0; i<n; i++)
        for (int j=0; j<m; j++)
            C[i][j] = add_mod(A[i][j], B[i][j]);
    return C;
}

vector<int> splice_vect(vector<int> &A,int K, int cnt){
    vector<int>ans;
    for(int i=0;i<K;i++) ans.push_back(A[cnt++]);
    return ans;
}