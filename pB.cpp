#include "common.hpp"
#include <chrono>
using namespace std::chrono;


#if !defined(ROLE_p0) && !defined(ROLE_p1)
#error "ROLE must be defined as ROLE_p0 or ROLE_p1"
#endif

//Global variables to store du-atallah constants
vector<vector<int>>X_rec_n;
vector<vector<int>>Y_rec_n;
vector<int>alphas_n;
vector<vector<int>>X_rec_k;
vector<vector<int>>Y_rec_k;
vector<int>alphas_k;
vector<int> X_rec_val;
vector<int> Y_rec_val;
vector<int> alphas_val;
int N,M,K,Q;


awaitable<void> recv_coroutine(tcp::socket& sock, uint32_t& out) {
    co_await boost::asio::async_read(sock, boost::asio::buffer(&out, sizeof(out)), use_awaitable);
}

// Setup connection to P2 (P0/P1 act as clients, P2 acts as server)
awaitable<tcp::socket> setup_server_connection(boost::asio::io_context& io_context, tcp::resolver& resolver) {
    tcp::socket sock(io_context);

    // Connect to P2
    auto endpoints_p2 = resolver.resolve("p2", "9002");
    co_await boost::asio::async_connect(sock, endpoints_p2, use_awaitable);

    co_return sock;
}

// Receive random value from P2
awaitable<uint32_t> recv_from_P2(tcp::socket& sock) {
    uint32_t received;
    co_await recv_coroutine(sock, received);
    co_return received;
}

awaitable<int> MPCDOT(tcp::socket& peer_sock, int role, vector<int>A,vector<int>B, int ind){
    vector<int>& X = (A.size() == N ? X_rec_n[ind] : X_rec_k[ind]);
    vector<int>& Y = (A.size() == N ? Y_rec_n[ind] : Y_rec_k[ind]);
    int alpha = (A.size() == N ? alphas_n[ind] : alphas_k[ind]);
    int sz    = (A.size() == N ? N : K);
    vector<int>X_rec(sz);
    vector<int>Y_rec(sz);
    vector<int>blind1=add_vectors(X,A);
    vector<int>blind2=add_vectors(Y,B);
    if(role==0){
        co_await boost::asio::async_write(peer_sock, boost::asio::buffer(blind1.data(), sz*sizeof(int)), use_awaitable);
        co_await boost::asio::async_read(peer_sock, boost::asio::buffer(X_rec.data(), sz*sizeof(int)), use_awaitable);
        co_await boost::asio::async_write(peer_sock, boost::asio::buffer(blind2.data(), sz*sizeof(int)), use_awaitable);
        co_await boost::asio::async_read(peer_sock, boost::asio::buffer(Y_rec.data(), sz*sizeof(int)), use_awaitable);
    }
    else{
        co_await boost::asio::async_read(peer_sock, boost::asio::buffer(X_rec.data(), sz*sizeof(int)), use_awaitable);
        co_await boost::asio::async_write(peer_sock, boost::asio::buffer(blind1.data(), sz*sizeof(int)), use_awaitable);
        co_await boost::asio::async_read(peer_sock, boost::asio::buffer(Y_rec.data(), sz*sizeof(int)), use_awaitable);
        co_await boost::asio::async_write(peer_sock, boost::asio::buffer(blind2.data(), sz*sizeof(int)), use_awaitable);
    }
    vector<int> temp=add_vectors(B,Y_rec);
    co_return add_mod(sub_mod(dot_vector(A,temp),dot_vector(Y,X_rec)),alpha);
}


awaitable<int> MPC(tcp::socket& peer_sock, int role, int A,int B, int ind){
   int X= X_rec_val[ind];
   int Y= Y_rec_val[ind];
   int alpha= alphas_val[ind];
   int blind1=X+A;
   int blind2=Y+B;
   int X_rec,Y_rec;
    if(role==0){
        co_await boost::asio::async_write(peer_sock, boost::asio::buffer(&blind1, sizeof(int)), use_awaitable);
        co_await boost::asio::async_read(peer_sock, boost::asio::buffer(&X_rec, sizeof(int)), use_awaitable);
        co_await boost::asio::async_write(peer_sock, boost::asio::buffer(&blind2, sizeof(int)), use_awaitable);
        co_await boost::asio::async_read(peer_sock, boost::asio::buffer(&Y_rec, sizeof(int)), use_awaitable);
    }
    else{
        co_await boost::asio::async_read(peer_sock, boost::asio::buffer(&X_rec, sizeof(int)), use_awaitable);
        co_await boost::asio::async_write(peer_sock, boost::asio::buffer(&blind1, sizeof(int)), use_awaitable);
        co_await boost::asio::async_read(peer_sock, boost::asio::buffer(&Y_rec, sizeof(int)), use_awaitable);
        co_await boost::asio::async_write(peer_sock, boost::asio::buffer(&blind2, sizeof(int)), use_awaitable);
    }
    int temp=add_mod(B,Y_rec);
    co_return add_mod(sub_mod(mul_mod(A,temp),mul_mod(Y,X_rec)),alpha);
}

awaitable<int> ex_values(tcp::socket& peer_sock, int role, int A){
    int rec;
    if(role==0){
        co_await boost::asio::async_write(peer_sock, boost::asio::buffer(&A, sizeof(int)), use_awaitable);
        co_await boost::asio::async_read(peer_sock, boost::asio::buffer(&rec, sizeof(int)), use_awaitable);
    }
    else{
        co_await boost::asio::async_read(peer_sock, boost::asio::buffer(&rec, sizeof(int)), use_awaitable);
        co_await boost::asio::async_write(peer_sock, boost::asio::buffer(&A, sizeof(int)), use_awaitable);
    }
    co_return rec;
}

// Setup peer connection between P0 and P1
awaitable<tcp::socket> setup_peer_connection(boost::asio::io_context& io_context, tcp::resolver& resolver) {
    tcp::socket sock(io_context);
#ifdef ROLE_p0
    auto endpoints_p1 = resolver.resolve("p1", "9001");
    co_await boost::asio::async_connect(sock, endpoints_p1, use_awaitable);
#else
    tcp::acceptor acceptor(io_context, tcp::endpoint(tcp::v4(), 9001));
    sock = co_await acceptor.async_accept(use_awaitable);
#endif
    co_return sock;
}

// ----------------------- Main protocol -----------------------
awaitable<void> run(boost::asio::io_context& io_context, int M, int N, int K, int Q,
boost::asio::executor_work_guard<
                        boost::asio::io_context::executor_type>& guard) {
    tcp::resolver resolver(io_context);
    int role = (
    #ifdef ROLE_p0
        0
    #else
        1
    #endif
    );
    // Step 1: connect to P2 and receive random value
    tcp::socket server_sock = co_await setup_server_connection(io_context, resolver);
    tcp::socket peer_sock = co_await setup_peer_connection(io_context, resolver);
 
    //Step-2: Recieve seed
    uint32_t seed = co_await recv_from_P2(server_sock);
    //cout<<"Peer "<<role<< " recieved seed from server "<<seed<<endl;
    std::mt19937 rng(seed);
    //Timing addition start
    // Timing files (role-specific)
    string prefix = "./data/times_role" + to_string(role) + "_";

    ofstream f_pre(prefix + "preprocess.txt");
    ofstream f_online(prefix + "online.txt");

    ofstream f_delta(prefix + "t_delta.txt");       // Step 2
    ofstream f_vjdelta(prefix + "t_vjdelta.txt");   // Step 3
    ofstream f_Uwrite(prefix + "t_Uwrite.txt");     // Step 4

    ofstream f_uidelta(prefix + "t_uidelta.txt");   // Step 7
    ofstream f_Vwrite(prefix + "t_Vwrite.txt");       // Step 7
    ofstream f_mat(prefix + "matrix_generation.txt");

    //timing addition end
    //Step-3: construct matrices
    auto t_mat_start = high_resolution_clock::now();
    vector<vector<int>> U = generate_matrix(M, K, rng);
    vector<vector<int>> V = generate_matrix(N, K, rng);
    auto t_mat_end = high_resolution_clock::now();
    f_mat << duration_cast<microseconds>(t_mat_end - t_mat_start).count() << "\n";
    //cout<<U[0][0]<<"Role :  "<<role<<".  "<<V[0][0]<<endl;
    //cout<<"Storing U to a file"<<endl;
    print_matrix(U,role,0);
    print_matrix(V,role,1);
    //Step-4: Read the share files
    vector<vector<int>>secret_targets;
    vector<int>user_indices;
    read_queries(role, user_indices);
    string name = "./data/queries_"+to_string(role)+".txt";
    

    vector<DPFKey> keys = read_DPFkey_from_file(name,Q);
    auto t_pre_start = high_resolution_clock::now(); //timing//

    for(auto key : keys){
        vector<string> words = Eval_all_indices(key,N);
        vector<int>row;
        for(auto word : words){
            int temp= (int)string_int(word);
            if(key.advise) temp=MOD-temp;
            row.push_back(temp);
        }
        secret_targets.push_back(row);
    }
    cout<<"number of queries are  "<<user_indices.size()<<endl;

    // for (size_t k = 0; k < user_indices.size(); k++) {
    //     cout << "Query " << k << ": i=" << user_indices[k]<< std::endl;
    // }
    //print_matrix(secret_targets);
    //cout<<"hiii"<<Q<<endl;


    //Step-5: recieve du-atallah shares
    for(int it=0;it<Q;it++){
        //cout<<"hiii"<<Q<<endl;
        for(int it2=0;it2<2*K;it2++){
            vector<int> temp1(N);
            vector<int> temp2(N);
            int alpha;
            // Read temp1
            co_await boost::asio::async_read(
                server_sock,
                boost::asio::buffer(temp1.data(), N * sizeof(int)),
                boost::asio::use_awaitable
            );

            // Read temp2
            co_await boost::asio::async_read(
                server_sock,
                boost::asio::buffer(temp2.data(), N * sizeof(int)),
                boost::asio::use_awaitable
            );

            // Read alpha (scalar int)
            co_await boost::asio::async_read(
                server_sock,
                boost::asio::buffer(&alpha, sizeof(int)),
                boost::asio::use_awaitable
            );
            //cout<<"lolololol"<<Q<<endl;
            // cout<<"received stuff.  ";
            // //print_vector(temp1);
            X_rec_n.push_back(temp1);
            Y_rec_n.push_back(temp2);
            alphas_n.push_back(alpha);
            int x,y,alp;
            co_await boost::asio::async_read(
                server_sock,
                boost::asio::buffer(&x, sizeof(int)),
                boost::asio::use_awaitable
            );

            // Read temp2
            co_await boost::asio::async_read(
                server_sock,
                boost::asio::buffer(&y, sizeof(int)),
                boost::asio::use_awaitable
            );

            // Read alpha (scalar int)
            co_await boost::asio::async_read(
                server_sock,
                boost::asio::buffer(&alp, sizeof(int)),
                boost::asio::use_awaitable
            );
            X_rec_val.push_back(x);
            Y_rec_val.push_back(y);
            alphas_val.push_back(alp);
        }
        vector<int> temp1(K);
        vector<int> temp2(K);
        int alpha;
        // Read temp1
        co_await boost::asio::async_read(
            server_sock,
            boost::asio::buffer(temp1.data(), K * sizeof(int)),
            boost::asio::use_awaitable
        );

        // Read temp2
        co_await boost::asio::async_read(
            server_sock,
            boost::asio::buffer(temp2.data(), K * sizeof(int)),
            boost::asio::use_awaitable
        );

        // Read alpha (scalar int)
        co_await boost::asio::async_read(
            server_sock,
            boost::asio::buffer(&alpha, sizeof(int)),
            boost::asio::use_awaitable
        );
        
        X_rec_k.push_back(temp1);
        Y_rec_k.push_back(temp2);
        alphas_k.push_back(alpha);
        //cout<<"Hiii"<<Q<<endl;
    }
    //cout<<"all duatallah stuff done"<<endl;
    //cout<<"X_rec_n's 1st element"<<endl;
    //print_vector(X_rec_n[0]);
    //cout<<"sizes of X_rec_n, Y_rec_n and alpha_n. "<<size(X_rec_k)<<size(Y_rec_k)<<size(alphas_k);

    //timing start
    auto t_pre_end = high_resolution_clock::now();
    f_pre << duration_cast<microseconds>(t_pre_end - t_pre_start).count() << "\n";
    //timing end

    //Initialise counters to access different set of random values each time
    int cnt_n=0;
    int cnt_k=0;
    int cnt_val=0;

    //Initialise files to write shares into for Checking
    std::string filename="./data/vjshares_" + std::to_string(role) + ".txt";
    std::ofstream f_vjshares(filename);

    filename= "./data/deltas_" + std::to_string(role) + ".txt";
    std::ofstream f_deltas(filename);

    filename= "./data/updated_ui" + std::to_string(role) + ".txt";
    std::ofstream f_uiupdates(filename);

    filename= "./data/updated_vj" + std::to_string(role) + ".txt";
    std::ofstream f_vjupdates(filename);

    //Step-6 Process queries
    for(int it=0;it<Q;it++){
        auto t_online_start = high_resolution_clock::now();

        // per-query microtimes
        long long t_delta_us = 0;
        long long t_vjdelta_us = 0;
        long long t_Uwrite_us = 0;
        long long t_uidelta_us = 0;
        long long t_Vsum_us = 0;
        long long t_Vcols_us = 0;

        //cout<<"running MPC for query.   "<<it<<endl;
        //MPC step:1 calculate shares of v_j
        //cout<<"hola1"<<endl;
        auto t0 = high_resolution_clock::now();
        vector<int> vj_shares(K,0);
        for(int it2=0;it2<K;it2++){
            //cout<<"hola2"<<endl;
            vector<int>col= col_extract(it2,V);
            //cout<<"size of column "<<col.size();
            //print_vector(col);
            vj_shares[it2]= co_await MPCDOT(peer_sock,role,col,secret_targets[it],cnt_n++);
        }
        //cout<<"Vj_shares";
        print_vector(vj_shares, f_vjshares);
        //cout<<"shares of v_j computed"<<endl;
        vector<int> ui_shares= U[user_indices[it]];

        //Step-2: Calculate delta
        int dotpdt= co_await MPCDOT(peer_sock,role,ui_shares,vj_shares,cnt_k++);
        int delta_sh= sub_mod(0,dotpdt);
        if(role==0) delta_sh=add_mod(delta_sh,1); //add 1 to delta share by exactly one client
        auto t1 = high_resolution_clock::now();
        t_delta_us += duration_cast<microseconds>(t1 - t0).count();
        f_deltas<<delta_sh<<"\n";

        //Step-3: v_j*delta + update U (no write yet)
        t0 = high_resolution_clock::now();
        vector<int> add_shares(K);
        for(int it2=0;it2<K;it2++){
            add_shares[it2]= co_await MPC(peer_sock,role,delta_sh,vj_shares[it2],cnt_val++);
        }
        vector<int>update=add_vectors(add_shares,ui_shares);
        t1 = high_resolution_clock::now();
        t_vjdelta_us += duration_cast<microseconds>(t1 - t0).count();
        print_vector(update, f_uiupdates);

        //Step-5: u_i*delta and update on V
        t0 = high_resolution_clock::now();
        vector<int>add_shares2(K);
        for(int it2=0;it2<K;it2++){
            add_shares2[it2]= co_await MPC(peer_sock,role,delta_sh,ui_shares[it2],cnt_val++);
        }
        vector<int>update2= add_vectors(vj_shares,add_shares2);
        t1 = high_resolution_clock::now();
        t_uidelta_us += duration_cast<microseconds>(t1 - t0).count();
        print_vector(update2,f_vjupdates);

        //Step-7 Update V matrix
        t0 = high_resolution_clock::now();
        for(int it2=0;it2<K;it2++){
            vector<int>Vcol= col_extract(it2,V);
            int M_share = add_shares2[it2];
            vector<string> eval = Eval_all_indices_M(keys[it],N);
            vector<int>values = string_int_vector(eval,0);
            vector<int>flags = string_int_vector(eval,1);
            if(role==1){
                for(int i=0;i<values.size();i++) values[i]=MOD-values[i];
            }
            int to_send = sub_mod(sum_vector(values),M_share);
            int recd = co_await ex_values(peer_sock,role,to_send);
            int fcw = add_mod(to_send,recd);
            if(role==0) fcw= MOD-fcw;
            for(int i =0; i<values.size(); i++){
                if(flags[i]==1) values[i]= add_mod(values[i],fcw);
            }
            col_update(it2,values,V);
        }
        t1 = high_resolution_clock::now();
        t_Vcols_us += duration_cast<microseconds>(t1 - t0).count();
        t0 = high_resolution_clock::now();
        U[user_indices[it]]= update; //update U matrix at end only effect in next iter step 8
        t1 = high_resolution_clock::now();
        t_Uwrite_us += duration_cast<microseconds>(t1 - t0).count();
        auto t_online_end = high_resolution_clock::now();
        f_online << duration_cast<microseconds>(t_online_end - t_online_start).count() << "\n";
        //cout<<"Update done"<<endl;
        f_delta   << t_delta_us   << "\n";
        f_vjdelta << t_vjdelta_us << "\n";
        f_Uwrite  << t_Uwrite_us  << "\n";
        f_uidelta << t_uidelta_us << "\n";
        f_Vwrite   << t_Vcols_us   << "\n";
    }

    //Do at the end Final handshake between peers to exit
    int value=-1;
    int out;
    cout<<"Exiting..."<<endl;
    if(role==0){
        co_await boost::asio::async_write(peer_sock, boost::asio::buffer(&value, sizeof(int)), use_awaitable);
        co_await boost::asio::async_read(peer_sock, boost::asio::buffer(&out, sizeof(int)), use_awaitable);
    }
    else{
        co_await boost::asio::async_read(peer_sock, boost::asio::buffer(&out, sizeof(int)), use_awaitable);
        co_await boost::asio::async_write(peer_sock, boost::asio::buffer(&value, sizeof(int)), use_awaitable);
    }
    guard.reset();
    co_return;
}

int main() {
    N= stoi(getenv("N"));
    M= stoi(getenv("M"));
    K= stoi(getenv("K"));
    Q= stoi(getenv("Q"));
    cout.setf(std::ios::unitbuf);
    boost::asio::io_context io_context(1);
    auto guard = boost::asio::make_work_guard(io_context);
    co_spawn(io_context, run(io_context,M,N,K,Q,guard), boost::asio::detached);
    io_context.run();
    return 0;
}
