// Simple snippet for the UCI loop
std::string command;
while (std::getline(std::cin, command)) {
    if (command == "uci") {
        std::cout << "id name MyEngine\n";
        std::cout << "uciok" << std::endl;
    } else if (command == "isready") {
        std::cout << "readyok" << std::endl;
    } else if (command == "go") {
        // Start your search thread here
        std::cout << "bestmove e2e4" << std::endl;
    }
}