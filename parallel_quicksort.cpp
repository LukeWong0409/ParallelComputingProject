#include <iostream>
#include <vector>
#include <algorithm>
#include <chrono>
#include <random>
#include <omp.h>

using namespace std;
using namespace std::chrono;

// 串行快速排序
void quicksort_serial(vector<int>& arr, int low, int high) {
    if (low < high) {
        int pivot = arr[high];
        int i = low - 1;
        
        for (int j = low; j < high; j++) {
            if (arr[j] <= pivot) {
                i++;
                swap(arr[i], arr[j]);
            }
        }
        swap(arr[i + 1], arr[high]);
        int pi = i + 1;
        
        quicksort_serial(arr, low, pi - 1);
        quicksort_serial(arr, pi + 1, high);
    }
}

// 并行快速排序
void quicksort_parallel(vector<int>& arr, int low, int high, int depth = 0) {
    if (low < high) {
        int pivot = arr[high];
        int i = low - 1;
        
        for (int j = low; j < high; j++) {
            if (arr[j] <= pivot) {
                i++;
                swap(arr[i], arr[j]);
            }
        }
        swap(arr[i + 1], arr[high]);
        int pi = i + 1;
        
        // 控制递归深度，避免创建过多线程
        if (depth < 4) {
            #pragma omp parallel sections
            {
                #pragma omp section
                quicksort_parallel(arr, low, pi - 1, depth + 1);
                
                #pragma omp section
                quicksort_parallel(arr, pi + 1, high, depth + 1);
            }
        } else {
            // 当递归深度较深时使用串行排序
            quicksort_serial(arr, low, pi - 1);
            quicksort_serial(arr, pi + 1, high);
        }
    }
}

// 生成随机数组
vector<int> generate_random_array(int size) {
    vector<int> arr(size);
    random_device rd;
    mt19937 gen(rd());
    uniform_int_distribution<> dis(1, 1000000);
    
    for (int i = 0; i < size; i++) {
        arr[i] = dis(gen);
    }
    
    return arr;
}

// 测试排序性能
void test_performance(int data_size, int num_threads) {
    cout << "测试数据量: " << data_size << "，线程数: " << num_threads << endl;
    
    // 设置线程数
    omp_set_num_threads(num_threads);
    
    // 生成随机数组
    vector<int> arr_serial = generate_random_array(data_size);
    vector<int> arr_parallel = arr_serial; // 复制数组用于并行排序
    
    // 串行排序计时
    auto start_serial = high_resolution_clock::now();
    quicksort_serial(arr_serial, 0, arr_serial.size() - 1);
    auto end_serial = high_resolution_clock::now();
    double time_serial = duration<double>(end_serial - start_serial).count();
    
    // 并行排序计时
    auto start_parallel = high_resolution_clock::now();
    quicksort_parallel(arr_parallel, 0, arr_parallel.size() - 1);
    auto end_parallel = high_resolution_clock::now();
    double time_parallel = duration<double>(end_parallel - start_parallel).count();
    
    // 验证排序结果
    bool correct = is_sorted(arr_serial.begin(), arr_serial.end()) && 
                   is_sorted(arr_parallel.begin(), arr_parallel.end()) &&
                   arr_serial == arr_parallel;
    
    if (correct) {
        cout << "排序结果正确!" << endl;
        cout << "串行排序时间: " << time_serial << " 秒" << endl;
        cout << "并行排序时间: " << time_parallel << " 秒" << endl;
        cout << "加速比: " << time_serial / time_parallel << endl;
    } else {
        cout << "排序结果错误!" << endl;
    }
    
    cout << "------------------------" << endl;
}

int main() {
    // 测试不同数据量
    vector<int> data_sizes = {1000, 5000, 10000, 100000};
    
    // 测试不同线程数
    vector<int> thread_counts = {1, 2, 4, 8, 16};
    
    cout << "并行快速排序算法性能测试" << endl;
    cout << "==========================" << endl;
    
    for (int data_size : data_sizes) {
        cout << "\n数据量: " << data_size << endl;
        cout << "------------------------" << endl;
        
        for (int threads : thread_counts) {
            test_performance(data_size, threads);
        }
    }
    
    return 0;
}