#pragma once
#include <utility>
#include <boost/asio.hpp>
#include <boost/asio/awaitable.hpp>
#include <boost/asio/use_awaitable.hpp>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <random>
#include <sstream>
#include<vector>
#include <chrono>
#include<math.h>

#define MOD  8191  // 2^num_bits - 1
#define pow2 16384ULL //2^14
#define num_bits 13
int DEBUG=0; //Debug Flag for cout statements

using boost::asio::awaitable;
using boost::asio::co_spawn;
using boost::asio::detached;
using boost::asio::use_awaitable;
using boost::asio::ip::tcp;
using std::vector;
using std::endl;
using std::cout;
using std::stoi;
using std::string;
using std::pair;
using std::runtime_error;
using std::ofstream;
using std::cerr;
using std::mt19937_64;
using std::ios;
using std::setw;
using std::ifstream;
using std::to_string;

namespace this_coro = boost::asio::this_coro;

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
void read_queries(int role,
                  std::vector<int>& user_indices) {
    //std::string filename_targets = "./data/queries_" + std::to_string(role) + ".txt";
    std::string filename_indices = "./data/queries_i.txt";

    //std::ifstream infile_targets(filename_targets);
    std::ifstream infile_indices(filename_indices);

    // if (!infile_targets.is_open()) {
    //     throw std::runtime_error("Failed to open " + filename_targets);
    // }
    if (!infile_indices.is_open()) {
        throw std::runtime_error("Failed to open " + filename_indices);
    }

    // ---- Read secret_targets (each line is N numbers) ----
    // std::string line;
    // while (std::getline(infile_targets, line)) {
    //     std::istringstream iss(line);
    //     std::vector<int> row(N);
    //     for (int j = 0; j < N; j++) {
    //         if (!(iss >> row[j])) {
    //             throw std::runtime_error("Malformed line in " + filename_targets);
    //         }
    //     }
    //     secret_targets.push_back(std::move(row));
    // }

    // ---- Read user_indices (one index per line) ----
    int i;
    while (infile_indices >> i) {
        user_indices.push_back(i);
    }
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

vector<int> col_extract(int k, vector<vector<int>>&mat){
    vector<int>ans;
    for(int i=0;i<mat.size();i++) ans.push_back(mat[i][k]);
    return ans;
}

void col_update(int k, vector<int>col, vector<vector<int>>&mat){
    for(int i=0;i<mat.size();i++) mat[i][k] += col[i];
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

int sum_vector(vector<int>A){
    int ans=0;
    for(auto it : A) ans=add_mod(ans,it);
    return ans;
}


struct DPFKey {
    string init;
    vector<string> cws;
    vector<pair<string,string>> flag_corr;
    string final_corr_word;
    bool advise;
};

uint64_t string_int(const string &bin) {
    uint64_t val = 0;
    for (char bit : bin) {
        if (bit != '0' && bit != '1')
            throw runtime_error("Seed must contain only 0s and 1s");
        val = (val << 1) | (bit - '0');
    }
    return val;
}

string int_string(uint64_t x) {
    string out(num_bits+1, '0');
    for (int i = num_bits; i >= 0; --i)
        out[num_bits - i] = ((x >> i) & 1) ? '1' : '0';
    return out;
}

pair<string, string> PRG_expand(const string &binary_seed) {
    uint64_t seed_val = string_int(binary_seed);
    mt19937_64 rng(seed_val);
    uint64_t out1 = rng() % pow2;
    uint64_t out2 = rng() % pow2;
    return { int_string(out1), int_string(out2) };
}

string int_string_path(uint64_t x) {
    string out(64, '0');
    for (int i = 63; i >= 0; --i)
        out[63 - i] = ((x >> i) & 1) ? '1' : '0';
    return out;
}

vector<int> string_int_vector(vector<string>&s, int mode){
    vector<int>ans;
    for(auto it : s){
        string temp;
        if(mode==0) temp= it.substr(0,num_bits);
        else temp= string(1,it[num_bits]);
        ans.push_back(string_int(temp));
    }
    return ans;
}

uint64_t random_key() {
    std::random_device rd;
    std::mt19937_64 gen(rd());
    std::uniform_int_distribution<int> dist(0, pow2-1);
    return (uint64_t)dist(gen);
}

string xor_strings(string a, string b){
    string c="";
    for (int i=0;i<a.size();i++){
        char next;
        next = a[i]==b[i]?'0':'1';
        c=c+ next;
    }
    return c;
}

int log2_ciel(int n) {
    if (n <= 1) return 0;
    int k = 0;
    uint64_t val = 1;
    while (val < n) {
        val <<= 1;
        k++;
    }
    return k;
}


string evalDPF(DPFKey key, int index, int DPF_size){
    string target= int_string_path((uint64_t)index);
    int levels= log2_ciel(DPF_size);
    target= target.substr(64 - levels);
    string curr = key.init;
    for(int i=0;i<levels;i++){
        pair<string,string> childs = PRG_expand(curr);
        string child, flag_corr, corr, to_update;
        corr= key.cws[i];
        if(target[i]=='0'){
            child= childs.first;
            flag_corr= (key.flag_corr[i]).first;
        }
        else{
            child= childs.second;
            flag_corr= (key.flag_corr[i]).second;            
        }
        to_update= child;
        if(curr[num_bits]=='1'){
            to_update = xor_strings(child, corr);
            to_update[num_bits] = xor_strings(string(1,child[num_bits]), flag_corr)[0];
        }
        curr= to_update;
    }
    string ans= curr;
    if(ans[num_bits]=='1') ans=xor_strings(ans,key.final_corr_word);
    return ans.substr(0,num_bits);
}

pair<DPFKey, DPFKey> generateDPF(int index, int size, uint64_t update_val){
    DPFKey key1, key2;
    //set initial keys
    string s1= int_string(random_key());
    string s2= int_string(random_key());
    string update= int_string(update_val*2); //As the LSB is flag bit keep that 0... hence multiply by 2
    string path= int_string_path((uint64_t)index);
    int levels= log2_ciel(size);
    //set init flag bits
    s1[num_bits]='0';
    s2[num_bits]='1';
    //add to DPF_key struct
    key1.init=s1;
    key2.init=s2;
    if(DEBUG) cout<<"levels are"<<levels<<endl;
    path= path.substr(64 - levels); //truncate path from 64 bits to level many bits
    if(DEBUG) cout<<"path to target  "<<path<<endl;
    string par1, par2;
    par1=s1;
    par2=s2;
    for(int i=0;i<levels;i++){
        pair<string,string> childs_1= PRG_expand(par1);
        pair<string,string> childs_2= PRG_expand(par2);
        string correction, left_corr, right_corr, update_par1, update_par2;
        //prepare the paths
        if(path[i]=='0'){
            correction = xor_strings(childs_1.second, childs_2.second);
            left_corr= xor_strings(xor_strings(string(1,childs_1.first[num_bits]),string(1,childs_2.first[num_bits])),"1");
            right_corr= xor_strings(string(1,childs_1.second[num_bits]),string(1,childs_2.second[num_bits]));
            update_par1= childs_1.first;
            update_par2 = childs_2.first;
            if(par1[num_bits]=='1'){
                update_par1= xor_strings(update_par1, correction);
                update_par1[num_bits]= xor_strings(left_corr,string(1,childs_1.first[num_bits]))[0];
            }
            else if (par2[num_bits]=='1'){
                update_par2= xor_strings(update_par2, correction);
                update_par2[num_bits]= xor_strings(left_corr,string(1,childs_2.first[num_bits]))[0];
            }
            else {
                //should never come here
                cout<<"issue with flags"<<endl;
                break;
            }
        }
        else{
            correction = xor_strings(childs_1.first, childs_2.first);
            left_corr= xor_strings(string(1,childs_1.first[num_bits]),string(1,childs_2.first[num_bits]));
            right_corr= xor_strings(xor_strings(string(1,childs_1.second[num_bits]),string(1,childs_2.second[num_bits])), "1");
            update_par1= childs_1.second;
            update_par2 = childs_2.second;
            if(par1[num_bits]=='1'){
                update_par1= xor_strings(update_par1, correction);
                update_par1[num_bits]= xor_strings(right_corr,string(1,childs_1.second[num_bits]))[0];
            }
            else{
                update_par2= xor_strings(update_par2, correction);
                update_par2[num_bits]= xor_strings(right_corr,string(1,childs_2.second[num_bits]))[0];
            }
        }
        (key1.cws).push_back(correction);
        (key1.flag_corr).push_back({left_corr,right_corr});
        (key2.cws).push_back(correction);
        (key2.flag_corr).push_back({left_corr,right_corr});
        par1= update_par1;
        par2= update_par2;
    }
    key1.final_corr_word= xor_strings(xor_strings(par1,par2),update);
    key2.final_corr_word= key1.final_corr_word;
    string ss1 = evalDPF(key1,index,size);
    string ss2 = evalDPF(key2,index,size);
    if(string_int(ss1)>string_int(ss2)){
        key1.advise=false;
        key2.advise=true;
    }
    else{
        key1.advise=true;
        key2.advise=false;
    }
    if(par1[num_bits]=='0') return {key2,key1};
    else return {key1,key2};
}


vector<string> Eval_all_indices(DPFKey Key, int size){
    //return {"",""};
    vector<string>curr;
    curr.push_back(Key.init);
    int levels= log2_ciel(size);
    //eval till final level
    for(int i=0;i<levels;i++){
        string corr;
        corr= (Key.cws)[i];
        pair<string,string> f_corr = (Key.flag_corr)[i];
        vector<string> curr_upd;
        for(const auto &it:curr){
            pair<string,string> childs= PRG_expand(it);
            string left_child= childs.first;
            string right_child = childs.second;
            if(it[num_bits]=='1'){
                left_child= xor_strings(childs.first,corr);
                right_child= xor_strings(childs.second,corr);
                left_child[num_bits]= xor_strings(string(1, childs.first[num_bits]),f_corr.first)[0];
                right_child[num_bits]= xor_strings(string(1, childs.second[num_bits]),f_corr.second)[0];
            }
            curr_upd.push_back(left_child);
            curr_upd.push_back(right_child);
        }
        curr=curr_upd;
    }
    //Final level eval with the 2 correction words
    vector<string>ans;
    //cout<<curr.size()<<endl;
    for(const auto &it:curr){
        string temp= it;
        if(it[num_bits]=='1') temp= xor_strings(it,Key.final_corr_word);
        //cout<<"reached here"<<endl;
        //Get rid of the flag bit
        temp= temp.substr(0,num_bits);
        ans.push_back(temp);
        if(ans.size()==size) break; //break after you have size many strings
    }
    //cout<<"reached here"<<endl;
    return ans;
}

void print_DPFKey_to_file(const DPFKey &key, const string &filename, int idx) {
    ofstream fout(filename, (idx == 0) ? ios::out : (ios::out | ios::app));
    if (!fout.is_open()) {
        cerr << "Could not open file " << filename << " for writing.\n";
        return;
    }

    fout << key.init << "\n";

    fout << key.cws.size() << "\n";
    for (const auto &cw : key.cws)
        fout << cw << "\n";

    fout << key.flag_corr.size() << "\n";
    for (const auto &p : key.flag_corr)
        fout << p.first << " " << p.second << "\n";

    fout << key.final_corr_word << "\n";
    fout<< key.advise <<"\n";
}

vector<string> Eval_all_indices_M(DPFKey Key, int size){
    //return {"",""};
    vector<string>curr;
    curr.push_back(Key.init);
    int levels= log2_ciel(size);
    //eval till final level
    for(int i=0;i<levels;i++){
        string corr;
        corr= (Key.cws)[i];
        pair<string,string> f_corr = (Key.flag_corr)[i];
        vector<string> curr_upd;
        for(const auto &it:curr){
            pair<string,string> childs= PRG_expand(it);
            string left_child= childs.first;
            string right_child = childs.second;
            if(it[num_bits]=='1'){
                left_child= xor_strings(childs.first,corr);
                right_child= xor_strings(childs.second,corr);
                left_child[num_bits]= xor_strings(string(1, childs.first[num_bits]),f_corr.first)[0];
                right_child[num_bits]= xor_strings(string(1, childs.second[num_bits]),f_corr.second)[0];
            }
            curr_upd.push_back(left_child);
            curr_upd.push_back(right_child);
        }
        curr=curr_upd;
    }
    //Final level eval with the 2 correction words
    vector<string>ans;
    for(auto it : curr){
        ans.push_back(it);
        if(ans.size()==size) break;
    }
    return ans;
}



vector<DPFKey> read_DPFkey_from_file(const string &filename, int Q) {
    ifstream fin(filename);
    if (!fin.is_open()) {
        cerr << "Could not open file " << filename << " for reading.\n";
        return {};
    }

    vector<DPFKey> keys;
    for (int q = 0; q < Q; ++q) {
        DPFKey key;
        if (!(fin >> key.init)) break;

        size_t cws_count;
        fin >> cws_count;
        key.cws.resize(cws_count);
        for (size_t i = 0; i < cws_count; ++i)
            fin >> key.cws[i];

        size_t flag_count;
        fin >> flag_count;
        key.flag_corr.resize(flag_count);
        for (size_t i = 0; i < flag_count; ++i)
            fin >> key.flag_corr[i].first >> key.flag_corr[i].second;

        fin >> key.final_corr_word;
        fin>> key.advise;

        keys.push_back(key);
    }

    if ((int)keys.size() != Q)
        cerr << "Warning: Expected " << Q << " DPFKeys but found " << keys.size() << ".\n";

    return keys;
}


void print_DPFKey_to_file_old(const DPFKey &key, const string &filename, int i) {

    ofstream fout(filename, (i == 0) ? ios::out : (ios::out | ios::app));
    if (!fout.is_open()) {
        cerr << "Could not open file " << filename << " for writing.\n";
        return;
    }

    fout << "==============================\n";
    fout << "DPF Key " << i+1 << "\n";
    fout << "==============================\n\n";

    // Initial seed
    fout << "[Initial Seed]\n";
    fout << key.init << "\n\n";

    // Correction words
    fout << "[Correction Words]\n";
    for (size_t i = 0; i < key.cws.size(); ++i)
        fout << "Level " << setw(2) << i << ": " << key.cws[i] << "\n";
    fout << "\n";

    // Flag correction bits
    fout << "[Flag Correction Bits]\n";
    for (size_t i = 0; i < key.flag_corr.size(); ++i)
        fout << "Level " << setw(2) << i
             << ": left=" << key.flag_corr[i].first
             << ", right=" << key.flag_corr[i].second << "\n";
    fout << "\n";

    // Final correction words
    fout << "[Final Correction Words]\n";
    fout << "Left : "  << key.final_corr_word  << "\n";

    fout.close();
    //cout << "DPFKey written to " << filename << endl;
}