#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <cmath>
#include <algorithm>
#include <iomanip>
#include <numeric>
// 计算加权平均值
double calculateWeightedAverage(const std::vector<double>& values, const std::vector<double>& weights) {
    double weightedSum = 0.0;
    double totalWeight = 0.0;
    for (size_t i = 0; i < values.size(); ++i) {
        weightedSum += values[i] * weights[i];
        totalWeight += weights[i];
    }
    return totalWeight > 0 ? weightedSum / totalWeight : 0.0;
}

// 定义一个结构体用于存储数据点及其偏移信息
struct DataPoint {
    size_t index;
    double value;
    double distance;
};

// 比较函数，用于按偏移量降序排列
bool compareByDistance(const DataPoint& a, const DataPoint& b) {
    return a.distance > b.distance;
}

// 计算每个数据点的权重，越远离平均值的数据赋予越小的权重，并且调整特定数据点的权重
std::vector<double> calculateWeights(const std::vector<double>& values) {
    std::vector<double> weights(values.size(), 1.0); // 默认权重为1
    if (values.size() <= 1) {
        return weights;
    }

    // 计算除去当前点外的平均值，并计算每个数据点的偏移量
    std::vector<DataPoint> dataPoints;
    for (size_t i = 0; i < values.size(); ++i) {
        double sum = 0.0;
        size_t count = 0;
        for (size_t j = 0; j < values.size(); ++j) {
            if (i != j) {
                sum += values[j];
                ++count;
            }
        }
        double avgWithoutCurrent = sum / count;
        double distance = std::abs(values[i] - avgWithoutCurrent);
        dataPoints.push_back({ i, values[i], distance });
    }

    // 按偏移量从大到小排序
    std::sort(dataPoints.begin(), dataPoints.end(), compareByDistance);

    // 选择前20%的偏移数据点并增加其权重
    size_t topPercentile = static_cast<size_t>(std::ceil(values.size() * 0.2));
    for (size_t i = 0; i < topPercentile && i < dataPoints.size(); ++i) {
        size_t idx = dataPoints[i].index;
        // 增加权重，使得该数据点更偏向平均值
        weights[idx] += 1.0; // 这里可以根据需要调整增量
    }

    // 归一化权重
    double sumWeights = std::accumulate(weights.begin(), weights.end(), 0.0);
    for (auto& w : weights) {
        w /= sumWeights;
    }
    return weights;
}

// 简化数据集，通过窗口化和加权平均
std::vector<std::pair<std::string, double>> simplifyData(
    const std::vector<std::string>& timestamps,
    const std::vector<double>& powerValues,
    size_t desiredPoints) {

    std::vector<std::pair<std::string, double>> simplifiedData;
    if (powerValues.empty()) return simplifiedData;

    size_t totalPoints = powerValues.size();
    size_t windowSize = static_cast<size_t>(std::ceil(static_cast<double>(totalPoints) / desiredPoints));

    for (size_t i = 0; i < desiredPoints; ++i) {
        size_t startIdx = i * windowSize;
        size_t endIdx = std::min(startIdx + windowSize, totalPoints);

        if (startIdx >= totalPoints) break;

        // 提取当前窗口的数据
        std::vector<double> windowValues(powerValues.begin() + startIdx, powerValues.begin() + endIdx);
        std::vector<std::string> windowTimestamps(timestamps.begin() + startIdx, timestamps.begin() + endIdx);

        // 计算窗口内的权重
        std::vector<double> weights = calculateWeights(windowValues);

        // 计算加权平均值
        double weightedAvg = calculateWeightedAverage(windowValues, weights);

        // 找到中间时间戳
        size_t midIndex = startIdx + (endIdx - startIdx) / 2;
        std::string midTimestamp = timestamps[midIndex];

        // 添加到简化后的数据集中
        simplifiedData.push_back({ midTimestamp, weightedAvg });
    }

    return simplifiedData;
}

int main() {
    // 打开原始CSV文件
    std::ifstream inputFile("Etest.csv");
    std::ofstream outputFile("Etest_processed.csv");

    // 检查文件是否成功打开
    if (!inputFile.is_open() || !outputFile.is_open()) {
        std::cerr << "Failed to open file." << std::endl;
        return 1;
    }

    // 跳过表头
    std::string header;
    std::getline(inputFile, header);

    // 存储时间戳和功率值
    std::vector<std::string> timestamps;
    std::vector<double> powerValues;

    // 读取CSV文件中的数据
    std::string line, timeStr;
    double power;
    while (std::getline(inputFile, line)) {
        std::istringstream ss(line);
        ss >> timeStr >> power;
        timestamps.push_back(timeStr);
        powerValues.push_back(power);
    }

    // 确保时间和功率值一一对应
    if (timestamps.size() != powerValues.size()) {
        std::cerr << "Error: Timestamp and power value counts do not match." << std::endl;
        return 1;
    }

    // 科学地简化数据集，保留指定数量的数据点
    const size_t DESIRED_POINTS = 100;
    auto simplifiedData = simplifyData(timestamps, powerValues, DESIRED_POINTS);

    // 写入新CSV文件的表头
    outputFile << "Time,Simplified_Power" << std::endl;

    // 将简化后的数据写入新的CSV文件
    for (const auto& dataPoint : simplifiedData) {
        outputFile << dataPoint.first << "," << std::fixed << std::setprecision(6) << dataPoint.second << std::endl;
    }

    // 关闭文件
    inputFile.close();
    outputFile.close();

    std::cout << "Data processing and simplification completed." << std::endl;
    return 0;
}