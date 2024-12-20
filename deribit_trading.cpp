#include <websocketpp/config/asio_client.hpp>
#include <websocketpp/client.hpp>
#include <nlohmann/json.hpp>
#include <iostream>
#include <string>
#include <memory>
#include <thread>
#include <chrono>
#include <map>
#include <functional>
#include <vector>
#include <iomanip>
#include <sstream>
#include <mutex>
#include <condition_variable>

using json = nlohmann::json;
using Client = websocketpp::client<websocketpp::config::asio_tls_client>;

class DeribitFullTrader {
public:
    DeribitFullTrader() :
        client_(),
        request_id_(1),
        is_authenticated_(false),
        is_connected_(false),
        show_subscription_updates_(true) {
        setupClient();
    }

    // Connection Management
    void connect(bool use_testnet = true) {
        std::string url = use_testnet ?
            "wss://test.deribit.com/ws/api/v2" :
            "wss://www.deribit.com/ws/api/v2";

        websocketpp::lib::error_code ec;
        connection_ = client_.get_connection(url, ec);

        if (ec) {
            throw std::runtime_error("Connection error: " + ec.message());
        }

        client_.connect(connection_);

        client_thread_ = std::thread([this]() {
            try {
                client_.run();
            }
            catch (const std::exception& e) {
                std::cerr << "WebSocket error: " << e.what() << std::endl;
            }
            });

        waitForConnection();
    }

    void disconnect() {
        if (connection_) {
            connection_->close(websocketpp::close::status::normal, "Closing connection");
        }
        if (client_thread_.joinable()) {
            client_thread_.join();
        }
    }

    // Authentication
    void authenticate(const std::string& client_id, const std::string& client_secret) {
        json auth_params = {
            {"grant_type", "client_credentials"},
            {"client_id", client_id},
            {"client_secret", client_secret}
        };

        sendRequest("public/auth", auth_params);
        waitForAuthentication();
    }

    // Public API Methods
    void getTime() {
        sendRequest("public/get_time", json::object());
    }

    void getInstruments(const std::string& currency, const std::string& kind) {
        json params = {
            {"currency", currency},
            {"kind", kind}
        };
        sendRequest("public/get_instruments", params);
    }

    void getCurrencies() {
        sendRequest("public/get_currencies", json::object());
    }

    void getOrderbook(const std::string& instrument_name, int depth = 5) {
        json params = {
            {"instrument_name", instrument_name},
            {"depth", depth}
        };
        sendRequest("public/get_order_book", params);
    }

    void getTradingviewChartData(const std::string& instrument_name,
        const std::string& start_timestamp,
        const std::string& end_timestamp,
        const std::string& resolution) {
        json params = {
            {"instrument_name", instrument_name},
            {"start_timestamp", start_timestamp},
            {"end_timestamp", end_timestamp},
            {"resolution", resolution}
        };
        sendRequest("public/get_tradingview_chart_data", params);
    }

    // Private API Methods - Account
    void getAccountSummary(const std::string& currency) {
        checkAuthentication();
        json params = {
            {"currency", currency}
        };
        sendPrivateRequest("private/get_account_summary", params);
    }

    void getPositions(const std::string& currency) {
        checkAuthentication();
        json params = {
            {"currency", currency}
        };
        sendPrivateRequest("private/get_positions", params);
    }

    // Private API Methods - Trading
    void placeBuyOrder(const std::string& instrument_name,
        double amount,
        double price = 0,
        const std::string& type = "limit") {
        checkAuthentication();
        json params = {
            {"instrument_name", instrument_name},
            {"amount", amount}
        };

        if (type == "market") {
            params["type"] = "market";
        }
        else {
            params["type"] = "limit";
            params["price"] = price;
            params["time_in_force"] = "good_til_cancelled";
            params["post_only"] = false;
            params["reduce_only"] = false;
        }

        sendPrivateRequest("private/buy", params);
    }

    void placeSellOrder(const std::string& instrument_name,
        double amount,
        double price = 0,
        const std::string& type = "limit") {
        checkAuthentication();
        json params = {
            {"instrument_name", instrument_name},
            {"amount", amount}
        };

        if (type == "market") {
            params["type"] = "market";
        }
        else {
            params["type"] = "limit";
            params["price"] = price;
            params["time_in_force"] = "good_til_cancelled";
            params["post_only"] = false;
            params["reduce_only"] = false;
        }

        sendPrivateRequest("private/sell", params);
    }

    void cancelOrder(const std::string& order_id) {
        checkAuthentication();
        json params = {
            {"order_id", order_id}
        };
        sendPrivateRequest("private/cancel", params);
    }

    void cancelAllOrders() {
        checkAuthentication();
        sendPrivateRequest("private/cancel_all", json::object());
    }

    void modifyOrder(const std::string& order_id,
        double amount,
        double price) {
        checkAuthentication();
        json params = {
            {"order_id", order_id},
            {"amount", amount},
            {"price", price}
        };
        sendPrivateRequest("private/edit", params);
    }

    void getOpenOrders(const std::string& instrument_name = "") {
        checkAuthentication();
        json params;
        if (!instrument_name.empty()) {
            params["instrument_name"] = instrument_name;
        }
        sendPrivateRequest("private/get_open_orders_by_instrument", params);
    }

    void getOrderHistory(const std::string& instrument_name = "") {
        checkAuthentication();
        json params;
        if (!instrument_name.empty()) {
            params["instrument_name"] = instrument_name;
        }
        sendPrivateRequest("private/get_order_history_by_instrument", params);
    }

    //Subscription Methods
    void subscribeToOrderbook(const std::string& instrument_name) {
        json params = {
            {"channels", {
                "book." + instrument_name + ".100ms"
            }}
        };
        sendRequest("public/subscribe", params);
    }

    void subscribeToTrades(const std::string& instrument_name) {
        std::string channel = "trades." + instrument_name + ".100ms";
        json params = {
            {"channels", {channel}}
        };
        sendRequest("public/subscribe", params);
        addSubscription(channel);
    }

    void subscribeToInstrument(const std::string& instrument_name) {
        std::string channel = "ticker." + instrument_name + ".100ms";
        json params = {
            {"channels", {channel}}
        };
        sendRequest("public/subscribe", params);
        addSubscription(channel);
    }

    

    // CLI Interface
    void startCLI() {
        displayHelp();
        std::string command;
        while (true) {
            std::cout << "\nDeribit> ";
            std::getline(std::cin, command);
            if (!command.empty()) {
                processCommand(command);
            }
        }
    }

private:
    Client client_;
    Client::connection_ptr connection_;
    std::thread client_thread_;
    int request_id_;
    std::string access_token_;
    bool is_authenticated_;
    bool is_connected_;
    std::mutex mutex_;
    std::condition_variable cv_;
    std::map<std::string, bool> active_subscriptions_;
    std::mutex subscription_mutex_;
    bool show_subscription_updates_;
    std::map<std::string, std::function<void(const json&)>> subscription_handlers_;
    std::mutex handlers_mutex_;

    void setupClient() {
        client_.clear_access_channels(websocketpp::log::alevel::all);
        client_.set_access_channels(websocketpp::log::alevel::connect);
        client_.set_access_channels(websocketpp::log::alevel::disconnect);

        client_.init_asio();

        client_.set_tls_init_handler([](websocketpp::connection_hdl) {
            return websocketpp::lib::make_shared<boost::asio::ssl::context>(
                boost::asio::ssl::context::tlsv12);
            });

        client_.set_message_handler([this](websocketpp::connection_hdl hdl,
            Client::message_ptr msg) {
                handleMessage(msg->get_payload());
            });

        client_.set_open_handler([this](websocketpp::connection_hdl hdl) {
            std::lock_guard<std::mutex> lock(mutex_);
            is_connected_ = true;
            cv_.notify_all();
            });
    }

    void waitForConnection() {
        std::unique_lock<std::mutex> lock(mutex_);
        cv_.wait(lock, [this] { return is_connected_; });
    }

    void waitForAuthentication() {
        std::unique_lock<std::mutex> lock(mutex_);
        cv_.wait(lock, [this] { return is_authenticated_; });
    }

    void checkAuthentication() {
        if (!is_authenticated_) {
            throw std::runtime_error("Not authenticated");
        }
    }

    void sendRequest(const std::string& method, const json& params) {
        json request = {
            {"jsonrpc", "2.0"},
            {"id", request_id_++},
            {"method", method},
            {"params", params}
        };

        try {
            client_.send(connection_, request.dump(), websocketpp::frame::opcode::text);
        }
        catch (const std::exception& e) {
            throw std::runtime_error("Failed to send request: " + std::string(e.what()));
        }
    }

    void sendPrivateRequest(const std::string& method, const json& params) {
        json request_params = params;
        request_params["access_token"] = access_token_;
        sendRequest(method, request_params);
    }
    void addSubscription(const std::string& channel) {
        std::lock_guard<std::mutex> lock(subscription_mutex_);
        active_subscriptions_[channel] = true;
        std::cout << "Subscribed to channel: " << channel << std::endl;
    }

    void removeSubscription(const std::string& channel) {
        std::lock_guard<std::mutex> lock(subscription_mutex_);
        active_subscriptions_[channel] = false;
        std::cout << "Unsubscribed from channel: " << channel << std::endl;
    }
    void handleSubscriptionUpdate(const json& params) {
        std::string channel = params["channel"];
        std::cout << "Subscription update for channel " << channel << ":\n";
        std::cout << params["data"].dump(2) << std::endl;
    }

    void handleMessage(const std::string& message) {
        try {
            json response = json::parse(message);

            // Handle authentication response
            if (response.contains("result") &&
                response["result"].contains("access_token")) {
                std::lock_guard<std::mutex> lock(mutex_);
                access_token_ = response["result"]["access_token"];
                is_authenticated_ = true;
                cv_.notify_all();
                std::cout << "Authentication successful!" << std::endl;
                return;
            }

            // Handle subscription messages
            if (response.contains("method") && response["method"] == "subscription") {
                handleSubscriptionUpdate(response["params"]);
                return;
            }

            // Handle regular responses
            if (response.contains("error")) {
                std::cerr << "Error: " << response["error"]["message"] << std::endl;
            }
            else if (response.contains("result")) {
                std::cout << "Result: " << response["result"].dump(2) << std::endl;
            }
        }
        catch (const std::exception& e) {
            std::cerr << "Error parsing message: " << e.what() << std::endl;
        }
    }
    


    // Display Methods
    void displayOrderbook(const json& data) {
        if (!data.contains("bids") || !data.contains("asks")) return;

        std::cout << "\nOrderbook for " << data["instrument_name"].get<std::string>() << "\n";
        std::cout << std::string(50, '=') << "\n";
        std::cout << std::left << std::setw(25) << "BIDS" << std::setw(25) << "ASKS" << "\n";
        std::cout << std::string(50, '-') << "\n";

        const auto& bids = data["bids"];
        const auto& asks = data["asks"];
        size_t max_size = std::max(bids.size(), asks.size());

        for (size_t i = 0; i < max_size && i < 10; ++i) {
            std::cout << std::fixed << std::setprecision(8);

            // Print bids
            if (i < bids.size()) {
                std::cout << std::setw(12) << bids[i][0].get<double>()
                    << " | "
                    << std::setw(10) << bids[i][1].get<double>();
            }
            else {
                std::cout << std::setw(25) << " ";
            }

            std::cout << " | ";

            // Print asks
            if (i < asks.size()) {
                std::cout << std::setw(12) << asks[i][0].get<double>()
                    << " | "
                    << std::setw(10) << asks[i][1].get<double>();
            }
            std::cout << "\n";
        }
        std::cout << std::string(50, '=') << "\n";
    }

    void displayTrades(const json& data) {
        std::cout << "\nTrade: "
            << "Price: " << data["price"]
            << " Amount: " << data["amount"]
            << " Direction: " << data["direction"] << "\n";
    }

    void displayTicker(const json& data) {
        std::cout << "\nTicker Update for " << data["instrument_name"] << ":\n"
            << "Last Price: " << data["last_price"] << "\n"
            << "Mark Price: " << data["mark_price"] << "\n"
            << "Best Bid: " << data["best_bid_price"] << "\n"
            << "Best Ask: " << data["best_ask_price"] << "\n";
    }

    void displayUserOrder(const json& data) {
        std::cout << "\nOrder Update:\n"
            << "Order ID: " << data["order_id"] << "\n"
            << "Status: " << data["order_state"] << "\n"
            << "Price: " << data["price"] << "\n"
            << "Amount: " << data["amount"] << "\n";
    }

    void displayUserTrade(const json& data) {
        std::cout << "\nTrade Update:\n"
            << "Trade ID: " << data["trade_id"] << "\n"
            << "Price: " << data["price"] << "\n"
            << "Amount: " << data["amount"] << "\n"
            << "Direction: " << data["direction"] << "\n";
    }

    void displayHelp() {
        std::cout << "\n=== Deribit Trading Commands ===\n"
            << "General:\n"
            << "  help                                    - Show this help\n"
            << "  quit                                    - Exit the program\n"
            << "\nMarket Data:\n"
            << "  book <instrument>                       - Get orderbook\n"
            << "  instruments <currency> <kind>           - List available instruments\n"
            << "  currencies                              - List available currencies\n"
            << "  time                                    - Get server time\n"
            << "\nTrading:\n"
            << "  buy <instrument> <amount> <price>       - Place buy order\n"
            << "  sell <instrument> <amount> <price>      - Place sell order\n"
            << "  cancel <order_id>                       - Cancel specific order\n"
            << "  cancelall                               - Cancel all orders\n"
            << "  modify <order_id> <amount> <price>      - Modify order\n"
            << "\nAccount:\n"
            << "  positions <currency>                    - View positions\n"
            << "  balance <currency>                      - Check account balance\n"
            << "  orders <instrument>                     - View open orders\n"
            << "  history <instrument>                    - View order history\n"
            << "\nSubscriptions:\n"
            << "  sub book <instrument>                   - Subscribe to orderbook\n"
            << "  sub trades <instrument>                 - Subscribe to trades\n"
            << "  sub ticker <instrument>                 - Subscribe to ticker\n"
            << "  list subs                              - List active subscriptions\n"
            << "=====================================\n";
    }

    void processCommand(const std::string& cmd) {
        auto tokens = splitCommand(cmd);
        if (tokens.empty()) return;

        try {
            const std::string& command = tokens[0];

            // General commands
            if (command == "help") {
                displayHelp();
            }
            else if (command == "quit") {
                std::cout << "Exiting...\n";
                exit(0);
            }
            else if (command == "list" && tokens.size() == 2 && tokens[1] == "subs") {
                listActiveSubscriptions();
            }
            // Market data commands
            else if (command == "book" && tokens.size() == 2) {
                getOrderbook(tokens[1]);
            }
            else if (command == "instruments" && tokens.size() == 3) {
                getInstruments(tokens[1], tokens[2]);
            }
            else if (command == "currencies") {
                getCurrencies();
            }
            else if (command == "time") {
                getTime();
            }
            // Trading commands
            else if (command == "buy" && tokens.size() >= 3) {
                std::string instrument_name = tokens[1];
                double amount = std::stod(tokens[2]);

                if (tokens.size() >= 4 && tokens[3] == "market") {
                    placeBuyOrder(instrument_name, amount, 0, "market");
                    std::cout << "Placing market buy order: " << instrument_name
                        << " Amount: " << amount << std::endl;
                }
                else if (tokens.size() >= 4) {
                    double price = std::stod(tokens[3]);
                    placeBuyOrder(instrument_name, amount, price, "limit");
                    std::cout << "Placing limit buy order: " << instrument_name
                        << " Amount: " << amount << " Price: " << price << std::endl;
                }
                else {
                    std::cout << "Invalid buy command format. Use:\n"
                        << "buy <instrument> <amount> market\n"
                        << "buy <instrument> <amount> <price>" << std::endl;
                }
            }
            else if (command == "sell" && tokens.size() >= 3) {
                std::string instrument_name = tokens[1];
                double amount = std::stod(tokens[2]);

                if (tokens.size() >= 4 && tokens[3] == "market") {
                    placeSellOrder(instrument_name, amount, 0, "market");
                    std::cout << "Placing market sell order: " << instrument_name
                        << " Amount: " << amount << std::endl;
                }
                else if (tokens.size() >= 4) {
                    double price = std::stod(tokens[3]);
                    placeSellOrder(instrument_name, amount, price, "limit");
                    std::cout << "Placing limit sell order: " << instrument_name
                        << " Amount: " << amount << " Price: " << price << std::endl;
                }
                else {
                    std::cout << "Invalid sell command format. Use:\n"
                        << "sell <instrument> <amount> market\n"
                        << "sell <instrument> <amount> <price>" << std::endl;
                }
            }
            else if (command == "cancel" && tokens.size() == 2) {
                cancelOrder(tokens[1]);
            }
            else if (command == "cancelall") {
                cancelAllOrders();
            }
            else if (command == "modify" && tokens.size() == 4) {
                modifyOrder(tokens[1], std::stod(tokens[2]), std::stod(tokens[3]));
            }
            // Account commands
            else if (command == "positions" && tokens.size() == 2) {
                getPositions(tokens[1]);
            }
            else if (command == "balance" && tokens.size() == 2) {
                getAccountSummary(tokens[1]);
            }
            else if (command == "orders") {
                std::string instrument = tokens.size() > 1 ? tokens[1] : "";
                getOpenOrders(instrument);
            }
            else if (command == "history") {
                std::string instrument = tokens.size() > 1 ? tokens[1] : "";
                getOrderHistory(instrument);
            }
            // Subscription commands
            else if (command == "sub" && tokens.size() >= 3) {
                const std::string& subType = tokens[1];
                if (subType == "book") {
                    subscribeToOrderbook(tokens[2]);
                }
                else if (subType == "trades") {
                    subscribeToTrades(tokens[2]);
                }
                else if (subType == "ticker") {
                    subscribeToInstrument(tokens[2]);
                }

            }
            else if (command == "list" && tokens.size() == 2 && tokens[1] == "subs") {
                listActiveSubscriptions();
            }

            else {
                std::cout << "Invalid command. Type 'help' for available commands.\n";
            }
        }
        catch (const std::exception& e) {
            std::cout << "Error processing command: " << e.what() << std::endl;
        }
    }

    std::vector<std::string> splitCommand(const std::string& cmd) {
        std::vector<std::string> tokens;
        std::stringstream ss(cmd);
        std::string token;
        while (ss >> token) {
            tokens.push_back(token);
        }
        return tokens;
    }

    void listActiveSubscriptions() {
        std::lock_guard<std::mutex> lock(subscription_mutex_);
        std::cout << "Active subscriptions:" << std::endl;
        for (const auto& sub : active_subscriptions_) {
            if (sub.second) {
                std::cout << "- " << sub.first << std::endl;
            }
        }
    }
};

int main() {
    try {
        DeribitFullTrader trader;

        // Get connection type from user
        std::string network_type;
        std::cout << "Connect to testnet? (y/n): ";
        std::getline(std::cin, network_type);
        bool use_testnet = (network_type == "y" || network_type == "Y");

        // Connect to appropriate network
        std::cout << "Connecting to Deribit " << (use_testnet ? "testnet" : "mainnet") << "...\n";
        trader.connect(use_testnet);

        // Get API credentials
        std::string client_id, client_secret;
        std::cout << "Enter client_id: ";
        std::getline(std::cin, client_id);
        std::cout << "Enter client_secret: ";
        std::getline(std::cin, client_secret);

        // Authenticate
        trader.authenticate(client_id, client_secret);

        // Start CLI
        std::cout << "\nStarting trading interface...\n";
        trader.startCLI();
    }
    catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}