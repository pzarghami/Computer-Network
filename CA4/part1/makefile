all: cl
	g++ -std=c++17 -pthread pc_A.cpp -o pc_a.out
	g++ -std=c++17 -pthread pc_B.cpp -o pc_b.out
	g++ -std=c++17 -pthread router.cpp -o r.out

cl:
	rm -f line_* main *.out main.out
