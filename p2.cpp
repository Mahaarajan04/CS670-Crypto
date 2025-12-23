#include "common.hpp"

using boost::asio::ip::tcp;

 
// Send a number to a client
boost::asio::awaitable<void> handle_client(tcp::socket &socket, const std::string& name, uint32_t seed) {
    co_await boost::asio::async_write(socket, boost::asio::buffer(&seed, sizeof(seed)), boost::asio::use_awaitable);
}

// Send vectors
boost::asio::awaitable<void> send_vectors(tcp::socket &socket, std::vector<int> &X, std::vector<int> &Y, int alpha) {
    co_await boost::asio::async_write(
        socket,
        boost::asio::buffer(X.data(), X.size() * sizeof(int)),
        boost::asio::use_awaitable
    );
    co_await boost::asio::async_write(
        socket,
        boost::asio::buffer(Y.data(), Y.size() * sizeof(int)),
        boost::asio::use_awaitable
    );
    co_await boost::asio::async_write(
        socket,
        boost::asio::buffer(&alpha, sizeof(int)),
        boost::asio::use_awaitable
    );
}

// Send values
boost::asio::awaitable<void> send_values(tcp::socket &socket, int X, int Y, int alpha) {
    co_await boost::asio::async_write(
        socket,
        boost::asio::buffer(&X, sizeof(int)),
        boost::asio::use_awaitable
    );
    co_await boost::asio::async_write(
        socket,
        boost::asio::buffer(&Y, sizeof(int)),
        boost::asio::use_awaitable
    );
    co_await boost::asio::async_write(
        socket,
        boost::asio::buffer(&alpha, sizeof(int)),
        boost::asio::use_awaitable
    );
}

// Run multiple coroutines in parallel
template <typename... Fs>
void run_in_parallel(boost::asio::io_context& io_context, Fs&&... funcs) {
    (boost::asio::co_spawn(io_context, funcs, boost::asio::detached), ...);
}

int main() {
    try {
        boost::asio::io_context io_context;

        tcp::acceptor acceptor(io_context, tcp::endpoint(tcp::v4(), 9002));

        // Accept clients
        tcp::socket socket_p0(io_context);
        acceptor.accept(socket_p0);

        tcp::socket socket_p1(io_context);
        acceptor.accept(socket_p1);

        //setting seeds for each peer
        uint32_t seed1 = random_uint32();
        uint32_t seed2= random_uint32();
        uint32_t seed= random_uint32();

        int M,N,K,Q;
        Q= stoi(getenv("Q"));
        N= stoi(getenv("N"));
        M= stoi(getenv("M"));
        K= stoi(getenv("K"));
        std::mt19937 rng(seed);
        //vector<vector<DA_mat>> mat = generate_DA_mat(Q,N,K,rng);

        // Launch all coroutines in parallel to send seed
        run_in_parallel(io_context,
            [&]() -> boost::asio::awaitable<void> {
                co_await handle_client(socket_p0, "P0", seed1);
            },
            [&]() -> boost::asio::awaitable<void> {
                co_await handle_client(socket_p1, "P1", seed2);
            }
        );
        io_context.run();
        io_context.restart();
        //cout<<"hiii1"<<endl;
        //Send du-atalla for each query to get v_j shares
        // Create timing file for P2
        ofstream f_p2("./data/times_p2_da_generation.txt");

        for(int it=0;it<Q;it++){
            auto t_da_start = std::chrono::high_resolution_clock::now();
            int alpha0;
            int alpha1;
            for(int it2=0;it2<2*K;it2++){
                //send 2*K as one for 
                vector<int>Y0=generate_vector(N,rng);
                vector<int>Y1= generate_vector(N,rng);
                vector<int>X0= generate_vector(N,rng);
                vector<int>X1= generate_vector(N,rng);
                int randomness= generate_int(rng);
                alpha0=add_mod(dot_vector(X0,Y1),randomness);
                alpha1=sub_mod(dot_vector(X1,Y0),randomness);
                //Send X0 to peer 0 and X1 to peer1
                //print_vector(X0);
                //print_vector(X1);
                //print_vector(Y0);
                //print_vector(Y1);
                run_in_parallel(io_context,
                    [&]() -> boost::asio::awaitable<void> {
                        co_await send_vectors(socket_p0, X0,Y0,alpha0);
                    },
                    [&]() -> boost::asio::awaitable<void> {
                        co_await send_vectors(socket_p1, X1,Y1,alpha1);
                    }
                );
                io_context.run();
                io_context.restart();
                int x0= generate_int(rng);
                int x1= generate_int(rng);
                int y0= generate_int(rng);
                int y1= generate_int(rng);
                randomness= generate_int(rng);
                alpha0=add_mod(mul_mod(x0,y1),randomness);
                alpha1=sub_mod(mul_mod(x1,y0),randomness);
                run_in_parallel(io_context,
                    [&]() -> boost::asio::awaitable<void> {
                        co_await send_values(socket_p0, x0,y0,alpha0);
                    },
                    [&]() -> boost::asio::awaitable<void> {
                        co_await send_values(socket_p1, x1,y1,alpha1);
                    }
                );
                io_context.run();
                io_context.restart();
                    //cout<<"hiii2"<<endl;
            }
            vector<int>Y0=generate_vector(K,rng);
            vector<int>Y1= generate_vector(K,rng);
            vector<int>X0= generate_vector(K,rng);
            vector<int>X1= generate_vector(K,rng);
            int randomness= generate_int(rng);
            alpha0=add_mod(dot_vector(X0,Y1),randomness);
            alpha1=sub_mod(dot_vector(X1,Y0),randomness);
            //Send X0 to peer 0 and X1 to peer1
            run_in_parallel(io_context,
                [&]() -> boost::asio::awaitable<void> {
                    co_await send_vectors(socket_p0, X0,Y0,alpha0);
                },
                [&]() -> boost::asio::awaitable<void> {
                    co_await send_vectors(socket_p1, X1,Y1,alpha1);
                }
            );
            io_context.run();
            io_context.restart();
            //cout<<"hiii3"<<endl;
            //cout<<"hiii4"<<endl;
            auto t_da_end = std::chrono::high_resolution_clock::now();
            long long us = std::chrono::duration_cast<std::chrono::microseconds>(t_da_end - t_da_start).count();
            f_p2 << us << "\n";
        }
        int out;
        run_in_parallel(io_context,
            [&]() -> boost::asio::awaitable<void> {
                co_await boost::asio::async_read(socket_p0, boost::asio::buffer(&out, sizeof(int)), use_awaitable);
            },
            [&]() -> boost::asio::awaitable<void> {
                co_await boost::asio::async_read(socket_p1, boost::asio::buffer(&out, sizeof(int)), use_awaitable);
            }
        );
        io_context.run();
        //cout<<"hiii5"<<endl;
        // co_await boost::asio::async_read(socket_p0, boost::asio::buffer(&out, sizeof(int)), use_awaitable);
        // co_await boost::asio::async_read(socket_p1, boost::asio::buffer(&out, sizeof(int)), use_awaitable);
        //cout<<"du for v_j and dots sent"<<endl;
        return 0;

    } catch (std::exception& e) {
        std::cerr << "Exception in P2: " << e.what() << "\n";
    }
}
