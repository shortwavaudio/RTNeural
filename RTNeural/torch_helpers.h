#pragma once

#include "model_loader.h"

namespace RTNeural
{
namespace torch_helpers
{
    namespace detail
    {
        /** Torch Conv1D layers store their kernel weights in reverse order. */
        template <typename T>
        void reverseKernels(std::vector<std::vector<std::vector<T>>>& conv_weights)
        {
            for(auto& channel_weights : conv_weights)
            {
                for(auto& kernel : channel_weights)
                {
                    std::reverse(kernel.begin(), kernel.end());
                }
            }
        }

        /** Transposes the rows and columns of a matrix stored as a 2D vector. */
        template <typename T>
        std::vector<std::vector<T>> transpose(const std::vector<std::vector<T>>& x)
        {
            auto outer_size = x.size();
            auto inner_size = x[0].size();
            std::vector<std::vector<T>> y(inner_size, std::vector<T>(outer_size, (T)0));

            for(size_t i = 0; i < outer_size; ++i)
            {
                for(size_t j = 0; j < inner_size; ++j)
                    y[j][i] = x[i][j];
            }

            return y;
        }

        /** Swaps the "r" and "z" indexes of a GRU layer weights. */
        template <typename T>
        void swap_rz(std::vector<std::vector<T>>& vec2d, int size)
        {
            for(auto& vec : vec2d)
                std::swap_ranges(vec.begin(), vec.begin() + size, vec.begin() + size);
        }
    }

    template <typename T, typename DenseType>
    void loadDense(const nlohmann::json& modelJson, const std::string& layerPrefix, DenseType& dense)
    {
        const std::vector<std::vector<T>> dense_weights = modelJson.at(layerPrefix + "weight");
        dense.setWeights(dense_weights);
        const std::vector<T> dense_bias = modelJson.at(layerPrefix + "bias");
        dense.setBias(dense_bias.data());
    }

    /** Loads a Conv1D layer from a JSON object containing a PyTorch state_dict. */
    template <typename T, typename Conv1DType>
    void loadConv1D(const nlohmann::json& modelJson, const std::string& layerPrefix, Conv1DType& conv)
    {
        std::vector<std::vector<std::vector<T>>> conv_weights = modelJson.at(layerPrefix + "weight");
        detail::reverseKernels(conv_weights);
        conv.setWeights(conv_weights);

        std::vector<T> conv_bias = modelJson.at(layerPrefix + "bias");
        conv.setBias(conv_bias);
    }

    /** Loads a GRU layer from a JSON object containing a PyTorch state_dict. */
    template <typename T, typename GRUType>
    void loadGRU(const nlohmann::json& modelJson, const std::string& layerPrefix, GRUType& gru)
    {
        // For the kernel and recurrent weights, PyTorch stores the weights similar to the
        // Tensorflow format, but transposed, and with the "r" and "z" indexes swapped.

        const std::vector<std::vector<T>> gru_ih_weights = modelJson.at(layerPrefix + "weight_ih_l0");
        auto wVals = detail::transpose(gru_ih_weights);
        detail::swap_rz(wVals, gru.out_size);
        gru.setWVals(wVals);

        const std::vector<std::vector<T>> gru_hh_weights = modelJson.at(layerPrefix + "weight_hh_l0");
        auto uVals = detail::transpose(gru_hh_weights);
        detail::swap_rz(uVals, gru.out_size);
        gru.setUVals(uVals);

        // PyTorch stores the GRU bias pretty much the same as TensorFlow as well,
        // just in two separate vectors. And again, we need to swap the "r" and "z" parts.

        const std::vector<T> gru_ih_bias = modelJson.at(layerPrefix + "bias_ih_l0");
        const std::vector<T> gru_hh_bias = modelJson.at(layerPrefix + "bias_hh_l0");
        std::vector<std::vector<T>> gru_bias { gru_ih_bias, gru_hh_bias };
        detail::swap_rz(gru_bias, gru.out_size);
        gru.setBVals(gru_bias);
    }

    /** Loads a LSTM layer from a JSON object containing a PyTorch state_dict. */
    template <typename T, typename LSTMType>
    void loadLSTM(const nlohmann::json& modelJson, const std::string& layerPrefix, LSTMType& lstm)
    {
        const std::vector<std::vector<T>> lstm_weights_ih = modelJson.at(layerPrefix + "weight_ih_l0");
        lstm.setWVals(detail::transpose(lstm_weights_ih));

        const std::vector<std::vector<T>> lstm_weights_hh = modelJson.at(layerPrefix + "weight_hh_l0");
        lstm.setUVals(detail::transpose(lstm_weights_hh));

        std::vector<T> lstm_bias_ih = modelJson.at(layerPrefix + "bias_ih_l0");
        std::vector<T> lstm_bias_hh = modelJson.at(layerPrefix + "bias_hh_l0");
        for(int i = 0; i < lstm_bias_ih.size(); ++i)
            lstm_bias_hh[(size_t)i] += lstm_bias_ih[(size_t)i];
        lstm.setBVals(lstm_bias_hh);
    }
}
}
