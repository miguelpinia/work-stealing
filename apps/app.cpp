#include "ws/lib.hpp"
#include "nlohmann/json.hpp"
#include <iostream>
#include <fstream>

int main() {
    int c = suma(1, 2);
    json values = experimentComplete(GraphType::TORUS_2D, 1000, false);
    std::ofstream file("results.json");
    file << std::setw(4) << values << std::endl;
    file.close();
    std::cout << c << std::endl;
    return 0;
}
