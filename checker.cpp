#include "common1.hpp"
#include <fstream>
#include <vector>
#include <iostream>
#include <sstream>
#include <string>
#include<unordered_map>
using namespace std;

vector<vector<int>> read_matrix(const string &filename) {
    ifstream f(filename);
    vector<vector<int>> mat;
    string line;
    while (getline(f, line)) {
        stringstream ss(line);
        vector<int> row;
        int val;
        while (ss >> val) row.push_back(val);
        if (!row.empty()) mat.push_back(row);
    }
    return mat;
}

unordered_map<string,string> load_env_file(const string &filename) {
    ifstream f(filename);
    unordered_map<string,string> env;
    string line;
    while (getline(f, line)) {
        if (line.empty() || line[0] == '#') continue; // skip empty/comments
        size_t eq = line.find('=');
        if (eq == string::npos) continue;
        string key = line.substr(0, eq);
        string val = line.substr(eq+1);
        env[key] = val;
    }
    return env;
}

int main() {
    auto env = load_env_file(".env");

    int N = stoi(env["N"]);
    int M = stoi(env["M"]);
    int K = stoi(env["K"]);
    int Q = stoi(env["Q"]);

    // cout << "Loaded params from .env: "
    //     << "N=" << N << " M=" << M << " K=" << K << " Q=" << Q << endl;
    // Load initial U, V
    auto U0 = read_matrix("./data/U_0.txt");
    auto U1 = read_matrix("./data/U_1.txt");
    auto V0 = read_matrix("./data/V_0.txt");
    auto V1 = read_matrix("./data/V_1.txt");

    vector<vector<int>> U(U0.size(), vector<int>(U0[0].size()));
    vector<vector<int>> V(V0.size(), vector<int>(V0[0].size()));

    for (size_t i = 0; i < U0.size(); i++){
        for (size_t j = 0; j < U0[i].size(); j++){
            U[i][j] = add_mod(U0[i][j], U1[i][j]);
            //cout<<U[i][j]<<"  ";
        }
        //cout<<endl;
    }
    //cout<<endl<<endl;
    for (size_t i = 0; i < V0.size(); i++){
        for (size_t j = 0; j < V0[i].size(); j++){
            V[i][j] = add_mod(V0[i][j], V1[i][j]);
            //cout<<V[i][j]<<"  ";
        }
        //cout<<endl;
    }

    // Load queries, deltas, vjshares, updated U
    auto j_shares_0 = read_matrix("./data/queries_0.txt");
    auto j_shares_1 = read_matrix("./data/queries_1.txt");
    auto i_mat = read_matrix("./data/queries_i.txt");
    auto j_mat = read_matrix("./data/queries_j.txt");
    auto D0 = read_matrix("./data/deltas_0.txt");
    auto D1 = read_matrix("./data/deltas_1.txt");
    auto VJ0 = read_matrix("./data/vjshares_0.txt");
    auto VJ1 = read_matrix("./data/vjshares_1.txt");
    auto Uupd0 = read_matrix("./data/updated_ui0.txt");
    auto Uupd1 = read_matrix("./data/updated_ui1.txt");
    auto Vupd0 = read_matrix("./data/updated_vj0.txt");
    auto Vupd1 = read_matrix("./data/updated_vj1.txt");

    cout << "Simulating queries...\n";
    int master=1;
    for (size_t q = 0; q < Q; q++) {
        // Reconstruct query
        int ind_i= i_mat[q][0];
        int ind_j= j_mat[q][0];

        //Check vj_shares consistency
        int flag=0;
        for(int i=0;i<K;i++){
            if(V[ind_j][i]!=add_mod(VJ0[q][i],VJ1[q][i])){
                cout<<"Failed query due to Vj_shares mismatch  "<<q<<endl;
                flag=1;
                master=0;
                break;
            }
        }
        if(flag) break;

        //Delta checking
        flag=0;
        int delta_org= sub_mod(1,dot_vector(U[ind_i],V[ind_j]));
        if(add_mod(D0[q][0],D1[q][0])!=delta_org){
            cout<<"Failed query due to delta mismatch  "<<q<<endl;
            flag=1;
            master=0;
        }
        if(flag) break;

        //Updated u_i checking
        vector<int> updated = add_vectors(mult_vector_scalar(V[ind_j],delta_org),U[ind_i]);
        flag=0;
        for(int i=0;i<K;i++){
            if(updated[i]!=add_mod(Uupd0[q][i],Uupd1[q][i])){
                cout<<"Failed query due to updated shares mismatch  "<<q<<endl;
                flag=1;
                master=0;
                break;
            }
        }
        if(flag) break;    
        
        //Update U matrix
        //Updated v_j checking
        vector<int>updated2 = add_vectors(mult_vector_scalar(U[ind_i],delta_org),V[ind_j]);
        flag=0;
        for(int i=0;i<K;i++){
            if(updated2[i]!=add_mod(Vupd0[q][i],Vupd1[q][i])){
                cout<<"Failed query due to updated shares V mismatch  "<<q<<endl;
                flag=1;
                master=0;
                break;
            }
        }
        if(flag) break;    
        
        //Update U, V matrix
        V[ind_j]=updated2;
        U[ind_i]=updated;
        //cout<<"Query executed successfully.   "<<q<<endl;
    }

    if(master) cout << "All queries processed.\n";
    return 0;
}
