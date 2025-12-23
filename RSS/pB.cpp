#include "common.hpp"


int N,M,K,Q;
vector<int>zero_shares;
int cnt=0;



awaitable<void> recv_coroutine(tcp::socket& sock, uint32_t& out) {
    co_await boost::asio::async_read(sock, boost::asio::buffer(&out, sizeof(out)), use_awaitable);
}

// Setup connection to P2 (P0/P1 act as clients, P2 acts as server)
awaitable<tcp::socket> setup_server_connection(boost::asio::io_context& io_context, tcp::resolver& resolver) {
    tcp::socket sock(io_context);

    // Connect to P2
    auto endpoints_p2 = resolver.resolve("dealer", "9002");
    co_await boost::asio::async_connect(sock, endpoints_p2, use_awaitable);

    co_return sock;
}

// Receive random value from P2
awaitable<uint32_t> recv_from_P2(tcp::socket& sock) {
    uint32_t received;
    co_await recv_coroutine(sock, received);
    co_return received;
}

// Setup peer connection between P0 and P1
awaitable<tcp::socket> setup_peer_connection_p01(boost::asio::io_context& io_context, tcp::resolver& resolver) {
    tcp::socket sock(io_context);
#ifdef ROLE_p0
    auto endpoints_p1 = resolver.resolve("rss_p1", "9001");
    co_await boost::asio::async_connect(sock, endpoints_p1, use_awaitable);
#else
    tcp::acceptor acceptor(io_context, tcp::endpoint(tcp::v4(), 9001));
    sock = co_await acceptor.async_accept(use_awaitable);
#endif
    cout<<"setup 01 done"<<endl;
    co_return sock;
}

awaitable<tcp::socket> setup_peer_connection_p12(boost::asio::io_context& io_context, tcp::resolver& resolver) {
    tcp::socket sock(io_context);
#ifdef ROLE_p1
    auto endpoints_p1 = resolver.resolve("rss_p2", "9001");
    co_await boost::asio::async_connect(sock, endpoints_p1, use_awaitable);
#else
    tcp::acceptor acceptor(io_context, tcp::endpoint(tcp::v4(), 9001));
    sock = co_await acceptor.async_accept(use_awaitable);
#endif
    cout<<"setup 12 done"<<endl;
    co_return sock;
}

awaitable<tcp::socket> setup_peer_connection_p02(boost::asio::io_context& io_context, tcp::resolver& resolver) {
    tcp::socket sock(io_context);
#ifdef ROLE_p0
    auto endpoints_p1 = resolver.resolve("rss_p2", "9001");
    co_await boost::asio::async_connect(sock, endpoints_p1, use_awaitable);
#else
    tcp::acceptor acceptor(io_context, tcp::endpoint(tcp::v4(), 9001));
    sock = co_await acceptor.async_accept(use_awaitable);
#endif
    cout<<"setup 02 done"<<endl;
    co_return sock;
}

awaitable<pair<vector<int>,vector<int>>> comm_vectors(tcp::socket& left, tcp::socket& right, int role, vector<int>A){
    vector<int>X= splice_vect(zero_shares,K,cnt);
    cnt+=K;
    vector<int>blind=add_vectors(X,A);
    int sz=K;
    vector<int> rec(K);
    co_await boost::asio::async_write(right, boost::asio::buffer(blind), use_awaitable);
    co_await boost::asio::async_read(left, boost::asio::buffer(rec), use_awaitable);
    pair<vector<int>,vector<int>>ans;
    ans.second=blind;
    ans.first=rec;
    co_return ans;
}

awaitable<pair<int,int>> comm_values(tcp::socket& left, tcp::socket& right, int role, int A){
    int X= zero_shares[cnt++];
    int blind=add_mod(X,A);
    int rec;
    co_await boost::asio::async_write(right, boost::asio::buffer(&blind, sizeof(int)), use_awaitable);
    co_await boost::asio::async_read(left, boost::asio::buffer(&rec, sizeof(int)), use_awaitable);
    pair<int,int>ans;
    ans.second=blind;
    ans.first=rec;
    co_return ans;
}



// ----------------------- Main protocol -----------------------
awaitable<void> run(boost::asio::io_context& io_context, int M, int N, int K, int Q,
boost::asio::executor_work_guard<
                        boost::asio::io_context::executor_type>& guard) {
    tcp::resolver resolver(io_context);
    int role = (
    #ifdef ROLE_p0
        0
    #elif ROLE_p1
        1
    #else
        2
    #endif
    );
    // Step 1: connect to P2 and receive random value
    tcp::socket server_sock = co_await setup_server_connection(io_context, resolver);
    tcp::socket left_sock(io_context);
    tcp::socket right_sock(io_context);
    cout<<"Connected to server"<<endl;
    if(role==0){
        left_sock = co_await setup_peer_connection_p02(io_context, resolver);
        right_sock = co_await setup_peer_connection_p01(io_context, resolver);
    }
    else if(role==1){
        left_sock = co_await setup_peer_connection_p01(io_context, resolver);
        right_sock = co_await setup_peer_connection_p12(io_context, resolver);        
    }
    else{
        right_sock = co_await setup_peer_connection_p02(io_context, resolver);
        left_sock = co_await setup_peer_connection_p12(io_context, resolver);
    }

    Msg m;
    co_await async_read(server_sock, boost::asio::buffer(&m, sizeof(m)),use_awaitable);
    std::cout << m.seed1 << ":" << m.seed2 << std::endl;

    std::mt19937 rng1(m.seed1);
    std::mt19937 rng2(m.seed2);
    cout<<m.seed1<<":"<<m.seed2<<endl;

    cout<<"Connections set up"<<endl;
    //receive zero_shares vector
    vector<int>temp(4*N*K*Q);
    co_await boost::asio::async_read(
        server_sock,
        boost::asio::buffer(temp.data(), N*K*Q*4*sizeof(int)),
        boost::asio::use_awaitable
    );
    zero_shares=temp;
    //cout<<"Vector recieved"<<endl;
    pair<vector<vector<int>>,vector<vector<int>>> U_shares;
    pair<vector<vector<int>>,vector<vector<int>>> V_shares;
    pair<vector<vector<int>>,vector<vector<int>>> j_shares;

    U_shares.second=generate_matrix(M,K,rng1);
    U_shares.first=generate_matrix(M,K,rng2);
    V_shares.second=generate_matrix(N,K,rng1);
    V_shares.first=generate_matrix(N,K,rng2);

    print_matrix(U_shares.first,role,0,0);
    print_matrix(U_shares.second,role,0,1);
    print_matrix(V_shares.first,role,1,0);
    print_matrix(V_shares.second,role,1,1);

    string filename= "./data/queries_" + std::to_string(role) + "_1.txt";
    std::ifstream f0(filename);
    filename= "./data/queries_" + std::to_string(role) + "_2.txt";
    std::ifstream f1(filename);
    j_shares.first=read_queries(N,f0);
    j_shares.second=read_queries(N,f1);

    vector<int>user_indices;
    filename="./data/queries_i.txt";
    std::ifstream infile_indices(filename);
    int i;
    while (infile_indices >> i) {
        user_indices.push_back(i);
    }
    //cout<<"Initialisations done"<<endl;

    //Checking files
    filename="./data/vjshares0_" + std::to_string(role) + ".txt";
    std::ofstream f_vjshares0(filename);

    filename= "./data/deltas0_" + std::to_string(role) + ".txt";
    std::ofstream f_deltas0(filename);

    filename= "./data/updated_ui0_" + std::to_string(role) + ".txt";
    std::ofstream f_uiupdates0(filename);
    filename="./data/vjshares1_" + std::to_string(role) + ".txt";
    std::ofstream f_vjshares1(filename);

    filename= "./data/deltas1_" + std::to_string(role) + ".txt";
    std::ofstream f_deltas1(filename);

    filename= "./data/updated_ui1_" + std::to_string(role) + ".txt";
    std::ofstream f_uiupdates1(filename);

    for(int q=0;q<Q;q++){
        //Compute vj_additive shares
        pair<vector<int>,vector<int>>Vjshares;
        int term1,term2,term3;
        vector<int>temp(K,0);
        for(int it = 0; it < K; it++) {
            vector<int> v0_col = col_extract(it, V_shares.first);
            vector<int> v1_col = col_extract(it, V_shares.second);
            //cout<<v0_col[0]<<".  "<<v1_col[0]<<endl;
            vector<int> j0 = j_shares.first[q];
            vector<int> j1 = j_shares.second[q];
            //cout<<j0[0]<<".  "<<j1[0]<<endl;
            term1 = dot_vector(v0_col, j0); 
            term2 = dot_vector(v1_col, j0);
            term3 = dot_vector(v0_col, j1); 
            int vj = add_mod(add_mod(term1, term2), term3);
            temp[it] = vj;
            //cout<<term1<<".  "<<term2<<".  "<<term3<<".  "<<vj<<endl;
        }

        Vjshares= co_await comm_vectors(left_sock,right_sock,role,temp);
        print_vector(Vjshares.first,f_vjshares0);
        print_vector(Vjshares.second,f_vjshares1);
        //cout<<"Vjshares computed"<<endl;
        pair<int,int> delta_shares;
        term1= dot_vector(Vjshares.first,U_shares.first[user_indices[q]]);
        term2= dot_vector(Vjshares.first,U_shares.second[user_indices[q]]);
        term3= dot_vector(Vjshares.second,U_shares.first[user_indices[q]]);
        int val1=sub_mod(0,add_mod(add_mod(term1, term2),term3));
        if(role==0) val1= add_mod(1,val1);
        delta_shares= co_await comm_values(left_sock,right_sock,role,val1);
        //cout<<"delta shares computed"<<endl;
        f_deltas0<<delta_shares.first<<"\n";
        f_deltas1<<delta_shares.second<<"\n";
        //compute delta*v_j
        pair<vector<int>,vector<int>> update;
        vector<int> upd(K);
        for(int it=0;it<K;it++){
            term1= mul_mod(Vjshares.first[it],delta_shares.first);
            term2= mul_mod(Vjshares.first[it],delta_shares.second);
            term3= mul_mod(Vjshares.second[it],delta_shares.first);
            upd[it]= add_mod(add_mod(term1, term2),term3);
        }
        update= co_await comm_vectors(left_sock,right_sock,role,upd);
        print_vector(update.first, f_uiupdates0);
        print_vector(update.second, f_uiupdates1);
        //cout<<"update calculated"<<endl;
        U_shares.first[user_indices[q]]=add_vectors(update.first,U_shares.first[user_indices[q]]);
        U_shares.second[user_indices[q]]=add_vectors(update.second,U_shares.second[user_indices[q]]);
        //cout<<"Query processed.  "<<q<<endl;
    }
    //server to exit
    cout<<"Exiting..."<<endl;
    int out=-1;
    co_await boost::asio::async_write(server_sock, boost::asio::buffer(&out, sizeof(int)), use_awaitable);
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
