#define _CRT_SECURE_NO_WARNINGS
#include <iostream>
#include <cmath>
#include <vector>
#include <random>
#include <algorithm>
#include <chrono>
#include <execution>
#include <iomanip>

//Berezhna Olga
//Group: K-27
//Variant 15

using namespace std;

double fast_operation(double x)
{
	return x * 1.1 + 2.5;
}

double slow_operation(double x)
{
	for (int i = 0; i < 100; i++)
	{
		x = sin(x) + cos(x);
	}
	return x;
}

template<typename Function>
double measure_transform_no_policy(const vector<double>& input, vector<double>& output, Function f)
{
	auto start = chrono::high_resolution_clock::now();
	transform(input.begin(), input.end(), output.begin(), f);
	auto end = chrono::high_resolution_clock::now();
	chrono::duration<double, milli> time = end - start;
	return time.count();
}

template<typename Policy,typename Function>
double measure_transform_with_policy(Policy&& policy, const vector<double>& input, vector<double>& output, Function f)
{
	auto start = chrono::high_resolution_clock::now();
	transform(policy, input.begin(), input.end(), output.begin(), f);
	auto end = chrono::high_resolution_clock::now();
	chrono::duration<double, milli> time = end - start;
	return time.count();
}

template <typename Function>
double custom_transform(const vector<double>& input, vector<double>& output, Function f, size_t K)
{
	size_t N = input.size();
	vector<thread> threads;
	threads.reserve(K);
	size_t chunk = N/K;

	auto start_time = chrono::high_resolution_clock::now();
	for (size_t i = 0; i < K; ++i)
	{
		size_t start = i * chunk;
		size_t end;
		if (i == K - 1) {
			end = N;
		}
		else {
			end = start + chunk;
		}
		threads.emplace_back([&, start, end]() {
		transform(input.begin() + start, input.begin() + end, output.begin() + start, f);
		});
	}
	for (auto& t : threads)
	{
		t.join();
	}
	auto end_time = chrono::high_resolution_clock::now();
	chrono::duration<double, milli> time = end_time - start_time;
	return time.count();

}
auto print_custom_results = [&](auto f, const vector<double>& data, vector<double>& result, const vector<size_t>& threads_count, unsigned int hardware_threads)
	{
		cout << "\n--- Custom parallel---\n";
		cout << "\n---Results table---\n";
		cout << "| K (threads) | time (ms) |\n";
		cout << "|------------|----------|\n";


		double best_time = 0.0;
		size_t best_K = 0;
		bool is_first = true; 

		for (auto K : threads_count)
		{
			if (K == 0) continue;
			double time = custom_transform(data, result, f, K);

			cout << "| " << setw(10) << K << " | " << setw(8) << time << " |\n";

			if (is_first) {
				best_time = time;
				best_K = K;
				is_first = false;
			}
			else if (time < best_time) {
				best_time = time;
				best_K = K;
			}
		}
		cout << "-----------------------------------------------------------------\n";
		cout << "The best speed is achieved at K = " << best_K << " (" << best_time << " ms)\n";

		double ratio = (double)best_K / hardware_threads;
		cout << "K/hardware_threads ratio: " << best_K << " / " << hardware_threads << " = " << ratio << "\n";

		cout << " Analysis of the dependence of time on K:\n";
		cout << "* For most tasks, the best speed is achieved when K is approximately equal to the hardware number of threads (" << hardware_threads << ") or a little more.\n";
		cout << "* At K  > " << hardware_threads << ": time usually increases (or acceleration stops) due to high overhead of context switching and managing a large number of threads.\n";
	};


int main()
{
	//connect depending on the level of optimization
	//freopen("result_0d.txt", "w", stdout);
	freopen("result_02.txt", "w", stdout);

	const unsigned int hardware_threads = thread::hardware_concurrency();
	cout << "=================================================================\n";
	cout << "Number of available hardware threads: " << hardware_threads << "\n";
	vector<size_t> sizes = { 100'000, 1'000'000};
	vector<size_t> threads_count = { 2,4,8 };
	mt19937 gen(random_device{}());
	uniform_real_distribution<double> range(0.0, 10.0);

	for (auto n : sizes)
	{
		cout << "\n=== Data size === " << n << "===\n";

		vector<double> data(n);
		generate(data.begin(), data.end(), [&]() {return range(gen); });
		vector <double> result(n);

		cout << "\n***FAST OPERATION***\n" << endl;
		cout << "No policy: "<< measure_transform_no_policy(data, result, fast_operation) << " ms\n";
		cout << "Policy SEQ: " << measure_transform_with_policy(std::execution::seq, data, result, fast_operation) << " ms\n";
		cout << "Policy PAR: " << measure_transform_with_policy(std::execution::par, data, result, fast_operation) << " ms\n";
		cout << "Policy PAR_UNSEQ: : " << measure_transform_with_policy(std::execution::par_unseq, data, result, fast_operation) << " ms\n";
		print_custom_results(fast_operation, data, result, threads_count, hardware_threads);
		
		cout << "\n***SLOW OPERATION***\n " << endl;
		cout << "No policy: " << measure_transform_no_policy(data, result, slow_operation) << " ms\n";
		cout << "Policy SEQ: " << measure_transform_with_policy(std::execution::seq, data, result, slow_operation) << " ms\n";
		cout << "Policy PAR: " << measure_transform_with_policy(std::execution::par, data, result, slow_operation) << " ms\n";
		cout << "Policy PAR_UNSEQ: : " << measure_transform_with_policy(std::execution::par_unseq, data, result, slow_operation) << " ms\n";
		print_custom_results(slow_operation, data, result, threads_count, hardware_threads);

	}	
	return 0;
}


