#include "common.hpp"

using boost::asio::ip::tcp;

 
// Send a number to a client
boost::asio::awaitable<void> handle_client(tcp::socket &socket, uint32_t seed1, uint32_t seed2) {
    Msg m{seed1, seed2};
    co_await async_write(socket, boost::asio::buffer(&m, sizeof(m)),use_awaitable);
}

// Send vectors
boost::asio::awaitable<void> send_vector(tcp::socket &socket, std::vector<int> &X) {
    co_await boost::asio::async_write(
        socket,
        boost::asio::buffer(X.data(), X.size() * sizeof(int)),
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

        int M,N,K,Q;
        Q= stoi(getenv("Q"));
        N= stoi(getenv("N"));
        M= stoi(getenv("M"));
        K= stoi(getenv("K"));
        boost::asio::io_context io_context;

        tcp::acceptor acceptor(io_context, tcp::endpoint(tcp::v4(), 9002));

        // Accept clients
        tcp::socket socket_p0(io_context);
        acceptor.accept(socket_p0);

        tcp::socket socket_p1(io_context);
        acceptor.accept(socket_p1);

        tcp::socket socket_p2(io_context);
        acceptor.accept(socket_p2);

        uint32_t seed, seed1, seed2, seed3;
        seed1 = random_uint32();
        seed2 = random_uint32();
        seed3 = random_uint32();
        seed = random_uint32();
        std::mt19937 rng(seed);

        // co_await handle_client(socket_p0,seed1);
        // co_await handle_client(socket_p0,seed2);
        // co_await handle_client(socket_p1,seed2);
        // co_await handle_client(socket_p1,seed3);
        // co_await handle_client(socket_p2,seed3);
        // co_await handle_client(socket_p2,seed1);

        run_in_parallel(io_context,
            [&]() -> boost::asio::awaitable<void> {
                co_await handle_client(socket_p0, seed1, seed2);
            },
            [&]() -> boost::asio::awaitable<void> {
                co_await handle_client(socket_p1, seed2, seed3);
            },
            [&]() -> boost::asio::awaitable<void> {
                co_await handle_client(socket_p2, seed3, seed1);
            }
        );
        io_context.run();
        io_context.restart();
        //making zero share vectors
        vector<vector<int>>zero_shares(3,vector<int>());
        for(int i=0;i<N*K*Q*4;i++){
            int s0= generate_int(rng);
            int s1= generate_int(rng);
            int s2= sub_mod(0,add_mod(s0,s1));
            zero_shares[0].push_back(s0);
            zero_shares[1].push_back(s1);
            zero_shares[2].push_back(s2);
        }
        
        run_in_parallel(io_context,
            [&]() -> boost::asio::awaitable<void> {
                co_await send_vector(socket_p0, zero_shares[0]);
            },
            [&]() -> boost::asio::awaitable<void> {
                co_await send_vector(socket_p1, zero_shares[1]);
            },
            [&]() -> boost::asio::awaitable<void> {
                co_await send_vector(socket_p2, zero_shares[2]);
            }
        );
        io_context.run();
        io_context.restart();



        int out;
        run_in_parallel(io_context,
            [&]() -> boost::asio::awaitable<void> {
                co_await boost::asio::async_read(socket_p0, boost::asio::buffer(&out, sizeof(int)), use_awaitable);
            },
            [&]() -> boost::asio::awaitable<void> {
                co_await boost::asio::async_read(socket_p1, boost::asio::buffer(&out, sizeof(int)), use_awaitable);
            },
            [&]() -> boost::asio::awaitable<void> {
                co_await boost::asio::async_read(socket_p2, boost::asio::buffer(&out, sizeof(int)), use_awaitable);
            }
        );
        io_context.run();
        return 0;

    } catch (std::exception& e) {
        std::cerr << "Exception in P2: " << e.what() << "\n";
    }
}
