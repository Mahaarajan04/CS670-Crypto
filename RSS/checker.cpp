#include "common1.hpp"
#include <fstream>
#include <vector>
#include <iostream>
#include <sstream>
#include <string>
#include <unordered_map>
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
    if (mat.empty()) {
        cerr << "Warning: file " << filename << " is empty!\n";
    }
    return mat;
}

unordered_map<string,string> load_env_file(const string &filename) {
    ifstream f(filename);
    unordered_map<string,string> env;
    string line;
    while (getline(f, line)) {
        if (line.empty() || line[0] == '#') continue;
        size_t eq = line.find('=');
        if (eq == string::npos) continue;
        string key = line.substr(0, eq);
        string val = line.substr(eq+1);
        env[key] = val;
    }
    return env;
}

inline int reconstruct3(int a, int b, int c) {
    return add_mod(add_mod(a, b), c);
}

int main() {
    auto env = load_env_file(".env");
    int N = stoi(env["N"]);
    int M = stoi(env["M"]);
    int K = stoi(env["K"]);
    int Q = stoi(env["Q"]);

    // ---------- Load U ----------
    auto U0_0 = read_matrix("./data/U_0_0.txt");
    auto U0_1 = read_matrix("./data/U_0_1.txt");
    auto U0_2 = read_matrix("./data/U_0_2.txt");

    auto U1_0 = read_matrix("./data/U_1_0.txt");
    auto U1_1 = read_matrix("./data/U_1_1.txt");
    auto U1_2 = read_matrix("./data/U_1_2.txt");

    vector<vector<int>> U(M, vector<int>(K));
    for (int i = 0; i < M; i++) {
        for (int j = 0; j < K; j++) {
            int u_set0 = reconstruct3(U0_0[i][j], U0_1[i][j], U0_2[i][j]);
            int u_set1 = reconstruct3(U1_0[i][j], U1_1[i][j], U1_2[i][j]);
            if (u_set0 != u_set1) {
                cerr << "Mismatch in U reconstruction at (" << i << "," << j << ")\n";
                return 1;
            }
            U[i][j] = u_set0;
        }
    }

    // ---------- Load V ----------
    auto V0_0 = read_matrix("./data/V_0_0.txt");
    auto V0_1 = read_matrix("./data/V_0_1.txt");
    auto V0_2 = read_matrix("./data/V_0_2.txt");

    auto V1_0 = read_matrix("./data/V_1_0.txt");
    auto V1_1 = read_matrix("./data/V_1_1.txt");
    auto V1_2 = read_matrix("./data/V_1_2.txt");

    vector<vector<int>> V(N, vector<int>(K));
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < K; j++) {
            int v_set0 = reconstruct3(V0_0[i][j], V0_1[i][j], V0_2[i][j]);
            int v_set1 = reconstruct3(V1_0[i][j], V1_1[i][j], V1_2[i][j]);
            if (v_set0 != v_set1) {
                cerr << "Mismatch in V reconstruction at (" << i << "," << j << ")\n";
                return 1;
            }
            V[i][j] = v_set0;
        }
    }

    cout << "U and V reconstructions consistent across sets.\n";

    // ---------- Load queries ----------
    auto i_mat = read_matrix("./data/queries_i.txt");
    auto j_mat = read_matrix("./data/queries_j.txt");

    // ---------- Load vjshares ----------
    auto VJ0_0 = read_matrix("./data/vjshares0_0.txt");
    auto VJ0_1 = read_matrix("./data/vjshares0_1.txt");
    auto VJ0_2 = read_matrix("./data/vjshares0_2.txt");

    auto VJ1_0 = read_matrix("./data/vjshares1_0.txt");
    auto VJ1_1 = read_matrix("./data/vjshares1_1.txt");
    auto VJ1_2 = read_matrix("./data/vjshares1_2.txt");

    // ---------- Load deltas ----------
    auto D0_0 = read_matrix("./data/deltas0_0.txt");
    auto D0_1 = read_matrix("./data/deltas0_1.txt");
    auto D0_2 = read_matrix("./data/deltas0_2.txt");

    auto D1_0 = read_matrix("./data/deltas1_0.txt");
    auto D1_1 = read_matrix("./data/deltas1_1.txt");
    auto D1_2 = read_matrix("./data/deltas1_2.txt");

    // ---------- Load updated_ui ----------
    auto Uupd0_0 = read_matrix("./data/updated_ui0_0.txt");
    auto Uupd0_1 = read_matrix("./data/updated_ui0_1.txt");
    auto Uupd0_2 = read_matrix("./data/updated_ui0_2.txt");

    auto Uupd1_0 = read_matrix("./data/updated_ui1_0.txt");
    auto Uupd1_1 = read_matrix("./data/updated_ui1_1.txt");
    auto Uupd1_2 = read_matrix("./data/updated_ui1_2.txt");

    // ---------- Simulate queries ----------
    cout << "Simulating queries...\n";
    int master = 1;

    for (int q = 0; q < Q; q++) {
        int ind_i = i_mat[q][0];
        int ind_j = j_mat[q][0];

        // --- Vj shares ---
        for (int k = 0; k < K; k++) {
            int vj_set0 = reconstruct3(VJ0_0[q][k], VJ0_1[q][k], VJ0_2[q][k]);
            int vj_set1 = reconstruct3(VJ1_0[q][k], VJ1_1[q][k], VJ1_2[q][k]);
            if (vj_set0 != vj_set1) {
                cerr << "Mismatch between Vj sets at q=" << q << " k=" << k << endl;
                master = 0; break;
            }
            if (V[ind_j][k] != vj_set0) {
                cerr << "Failed query " << q << " due to Vj mismatch\n";
                master = 0; break;
            }
        }
        if (!master) break;

        // --- Delta ---
        int delta_org = sub_mod(1, dot_vector(U[ind_i], V[ind_j]));
        int delta_set0 = reconstruct3(D0_0[q][0], D0_1[q][0], D0_2[q][0]);
        int delta_set1 = reconstruct3(D1_0[q][0], D1_1[q][0], D1_2[q][0]);
        if (delta_set0 != delta_set1) {
            cerr << "Mismatch between delta sets at q=" << q << endl;
            master = 0; break;
        }
        if (delta_org != delta_set0) {
            cerr << "Failed query " << q << " due to delta mismatch\n";
            master = 0; break;
        }

        // --- Updated U ---
        vector<int> updated = mult_vector_scalar(V[ind_j], delta_org);
        for (int k = 0; k < K; k++) {
            int upd_set0 = reconstruct3(Uupd0_0[q][k], Uupd0_1[q][k], Uupd0_2[q][k]);
            int upd_set1 = reconstruct3(Uupd1_0[q][k], Uupd1_1[q][k], Uupd1_2[q][k]);
            if (upd_set0 != upd_set1) {
                cerr << "Mismatch between updated_ui sets at q=" << q << " k=" << k << endl;
                master = 0; break;
            }
            if (updated[k] != upd_set0) {
                cout<<upd_set0<<".   "<<updated[k]<<endl;
                cerr << "Failed query " << q << " due to updated_ui mismatch\n";
                master = 0; break;
            }
        }
        if (!master) break;

        U[ind_i] = add_vectors(updated,U[ind_i]);
    }

    if (master) cout << "All queries processed successfully.\n";
    else cerr << "Checker failed.\n";
    return 0;
}
