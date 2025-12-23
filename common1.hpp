#pragma once
#include <iostream>
#include <fstream>
#include <random>
#include<vector>
#include <chrono>
#include<math.h>

#define MOD 8191


using std::vector;
using std::endl;
using std::cout;
using std::stoi;


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

void print_matrix(const std::vector<std::vector<int>>& mat, int role, int flag) {
    std::string filename;
    if(flag==0){
        filename = "./data/U_" + std::to_string(role) + ".txt";
    }
    else{
        filename = "./data/V_" + std::to_string(role) + ".txt";
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

vector<int> add_vectors(vector<int>A, vector<int>B){
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

vector<int> mult_vector_scalar(vector<int>&A, int B){
    vector<int>C;
    for(int i=0; i<A.size(); i++){
        C.push_back(mul_mod(A[i],B));
    }
    return C;
}

void print_vector(vector<int>A, std::ofstream &f){
    for(int i=0;i<A.size();i++) f<<A[i]<<"  ";
    f<<"\n";
}

int dot_vector(vector<int>&A, vector<int>&B){
    int ans=0;
    for(int i=0;i<A.size();i++) ans=add_mod(ans,mul_mod(A[i],B[i]));
    return ans;
}