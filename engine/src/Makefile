CXX = g++
CXXFLAGS = -std=c++17 -O2 \
	-I/opt/homebrew/Cellar/asio/1.36.0/include \
	-I/opt/homebrew/Cellar/nlohmann-json/3.12.0/include

engine: main.cpp
	$(CXX) $(CXXFLAGS) -o engine main.cpp -lpthread

clean:
	rm -f engine
