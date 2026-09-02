#pragma once
#include <vector>
#include <cmath>      // exp, sqrt
#include <algorithm>  // max
#include <random>     // mt19937, uniform_real_distribution
#include <chrono>     // steady_clock
#include <fstream>    // ifstream, ofstream
#include <iomanip>    // setprecision
#include <string>
#include <filesystem>
#define F(I,K) for(int I = 0; I<(int)K;I++)
#define F1(I,K) for(int I = 1; I<=(int)K;I++)
using namespace std;

inline mt19937 rng(chrono::steady_clock::now().time_since_epoch().count());

// Parameters of the neural network

const int encodeSize = 6;
const int histryWindow = 6;
const int inLayerSize = encodeSize * histryWindow;
const int l1Neurons = encodeSize * histryWindow;
const int l2Neurons = 96;
const int l3Neurons = 48;
const int outputNeurons = 3;
const int maxLayerSize = 1024;  
const float learningRate = 0.001;
const int layersNum = 3;
const float eps = 1e-8;

// Adam
const float beta1 = 0.2;
const float beta2 = 0.999;
inline int adamStep = 0;
inline float beta1_pow = 1.0;
inline float beta2_pow = 1.0;

inline float sigmoid(float x){
    return 1.0 / (1.0 + exp(-x));
}
inline float dsigmoid(float x){
    return x * (1.0 - x);
}
inline float relu(float x){
    return max(0.0f, x);
}
inline float drelu(float x){
    return x > 0 ? 1.0 : 0.0;
}
inline float elu(float x){
    return x > 0 ? x : exp(x) - 1.0;
}
inline float delu(float x){
    return x > 0 ? 1 : exp(x);
}

inline void softmax(vector<float>& x) {
    float maxVal = *max_element(x.begin(), x.end());
    float sum = 0.0f;
    for (float& v : x) { v = exp(v - maxVal); sum += v; }
    for (float& v : x) v /= sum;
}

inline float computeL2Norm(vector<float>& gradients){
    float norm = 0.0;
    for(float g : gradients){
        norm += g * g;
    }
    return sqrt(norm);
}

inline void clipGradients(vector<float>& gradients, float maxNorm){
    float currentNorm = computeL2Norm(gradients);
    if(currentNorm > maxNorm){
        float scale = maxNorm / (currentNorm + eps);;
        for(float& g : gradients){
            g *= scale;
        }
    }
}

struct Matrix{
    int rows;
    int cols;
    vector<float> data;

    Matrix(int rows, int cols) : rows(rows), cols(cols), data(rows * cols) {}

    float& operator()(int r, int c) {
        return data[r * cols + c];
    }
};

struct Layer {
    Matrix weights;
    vector<float> bias;

    // While forward
    vector<float> neuronsInput;
    vector<float> neuronsValues;
    vector<float> neuronsOutput;

    // While backward
    vector<float> inputGradients;
    Matrix weightGradients;
    vector<float> biasGradients;

    Matrix mWeigths, vWeights;
    vector<float> mBias, vBias;

    Layer(int inSize, int outSize) : weights(outSize, inSize), bias(outSize),
      neuronsValues(outSize), neuronsOutput(outSize),
      inputGradients(inSize), weightGradients(outSize, inSize), biasGradients(outSize),
      mWeigths(outSize, inSize), vWeights(outSize, inSize), mBias(outSize), vBias(outSize) {}

    void forward(vector<float> &input, bool activationFunction = 1){
        neuronsInput = input;

        F(i, weights.rows){
            neuronsValues[i] = bias[i];
            F(j,weights.cols){
                neuronsValues[i] += input[j] * weights(i,j);
            }

            if(activationFunction) neuronsOutput[i] = relu(neuronsValues[i]);
            else neuronsOutput[i] = neuronsValues[i];
        }
    }

    void backward(vector<float> &outputGradients, bool activationFunction = 1){
        int output_size = weights.rows;
        int input_size = weights.cols;

        // gradient of weights and bias
        F(i, output_size){
            biasGradients[i] = activationFunction ? outputGradients[i] * drelu(neuronsOutput[i]) : outputGradients[i];
            F(j, input_size){
                weightGradients(i,j) = activationFunction ? outputGradients[i] * drelu(neuronsOutput[i]) * neuronsInput[j] : outputGradients[i] * neuronsInput[j];
            }
        }

        // gradeints for next layer
        inputGradients.assign(input_size, 0.0f);
        F(j, output_size){
            F(i, input_size){
                inputGradients[i] += (activationFunction ? outputGradients[j] * drelu(neuronsOutput[j]) : outputGradients[j]) * weights(j,i);
            }
        }
    }

    void update(float &beta1_pow, float &beta2_pow){
        clipGradients(biasGradients, sqrt(biasGradients.size()));
        clipGradients(weightGradients.data, sqrt(weightGradients.rows * weightGradients.cols));

        F(i, weights.rows){

            mBias[i] = beta1 * mBias[i] + (1.0 - beta1) * biasGradients[i];
            vBias[i] = beta2 * vBias[i] + (1.0 - beta2) * biasGradients[i] * biasGradients[i];
            
            float mHat = mBias[i] / (1.0 - beta1_pow);
            float vHat = vBias[i] / (1.0 - beta2_pow);
            bias[i] -= learningRate * mHat / (sqrt(vHat) + eps);
            // bias[i] -= learningRate * biasGradients[i];
            
            F(j, weights.cols){
                mWeigths(i,j) = beta1 * mWeigths(i,j) + (1.0 - beta1) * weightGradients(i,j);
                vWeights(i,j) = beta2 * vWeights(i,j) + (1.0 - beta2) * weightGradients(i,j) * weightGradients(i,j);

                float mHatW = mWeigths(i,j) / (1.0 - beta1_pow);
                float vHatW = vWeights(i,j) / (1.0 - beta2_pow);
                weights(i,j) -= learningRate * mHatW / (sqrt(vHatW) + eps);
                // weights(i,j) -= learningRate * weightGradients(i,j);
            }
        }
    }
};

struct Network
{
    Layer l1,l2,l3;

    Network() : l1(l1Neurons,l2Neurons), l2(l2Neurons,l3Neurons), l3(l3Neurons,outputNeurons) {} // Network config

    void forwardPropagation(vector<float>& input) {
        // Forward propagation through the network
        l1.forward(input);
        l2.forward(l1.neuronsOutput);
        l3.forward(l2.neuronsOutput, 0);
        softmax(l3.neuronsOutput);
    }

    void backPropagation(vector<float> &gradient){
        // Backward propagation through the network
        l3.backward(gradient, 0);
        l2.backward(l3.inputGradients);
        l1.backward(l2.inputGradients);
    }

    void updateWeigths(){
        adamStep++;

        beta1_pow *= beta1;
        beta2_pow *= beta2;

        l1.update(beta1_pow, beta2_pow);
        l2.update(beta1_pow, beta2_pow);
        l3.update(beta1_pow, beta2_pow);
    }
};

inline void genNet(Network &net){
    
    F(i,layersNum){
        Layer* layer;
        if(i == 0) layer = &net.l1;
        else if(i == 1) layer = &net.l2;
        else if(i == 2) layer = &net.l3;

        F(r,layer->weights.rows){
            F(c,layer->weights.cols){
                layer->weights(r,c) = uniform_real_distribution<float>(-0.1,0.1)(rng);
            }
        }
        F(b,layer->bias.size()){
            layer->bias[b] = uniform_real_distribution<float>(-0.1,0.1)(rng);
        }
    }
}

inline void saveNet(Network &net, string filename = "users/defaultNet.txt"){
    ofstream fout(filename);
    fout << setprecision(9);
    F(i,layersNum){
        Layer* layer;
        if(i == 0) layer = &net.l1;
        else if(i == 1) layer = &net.l2;
        else if(i == 2) layer = &net.l3;

        F(r,layer->weights.rows){
            F(c,layer->weights.cols){
                fout << layer->weights(r,c) << " ";
            }
            fout << endl;
        }
        F(b,layer->bias.size()){
            fout << layer->bias[b] << " ";
        }
        fout << endl;
    }

    fout.close();
}

inline void loadNet(Network &loaded, string filename = "users/defaultNet.txt"){
    ifstream fin(filename);
    
    if(!fin.good()){
        fin.close();
        ifstream seed("data/seed.txt");
        if(seed.good()){
            seed.close();
            loadNet(loaded, "data/seed.txt");
        } else {
            genNet(loaded);
        }
        saveNet(loaded, filename);
        return;
    }

    F(i,layersNum){
        Layer* layer;
        if(i == 0) layer = &loaded.l1;
        else if(i == 1) layer = &loaded.l2;
        else if(i == 2) layer = &loaded.l3;

        F(r,layer->weights.rows){
            F(c,layer->weights.cols){
                fin >> layer->weights(r,c);
            }
        }
        F(b,layer->bias.size()){
            fin >> layer->bias[b];
        }
    }
    fin.close();
}