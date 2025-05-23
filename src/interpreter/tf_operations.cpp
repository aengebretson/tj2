// filepath: /Users/andrew/git/tj2/src/interpreter/tf_operations.cpp
#include "tf_operations.hpp"
#include <iostream>
#include <cassert>
#include <numeric>
#include <algorithm>
#include <variant> // Added for std::holds_alternative and std::get

namespace JInterpreter {

// Define the template specializations first, before they're used
template<typename T>
std::vector<T> JTensor::get_flat() const {
    // This is a generic template, it should be specialized.
    // However, to avoid linker errors for types not explicitly specialized,
    // we can provide a default implementation that throws or returns an empty vector.
    throw std::runtime_error("JTensor::get_flat() called with an unsupported type.");
}

template<>
std::vector<long long> JTensor::get_flat<long long>() const {
#ifdef TENSORFLOW_ENABLED
    if (m_dtype != DataType::INT64) {
        throw std::runtime_error("Attempting to get long long flat data from a non-INT64 tensor.");
    }
    if (!m_has_tf_tensor) {
        return {};
    }
    auto flat_tensor = m_tf_tensor.flat<tensorflow::int64>();
    return std::vector<long long>(flat_tensor.data(), flat_tensor.data() + flat_tensor.size());
#else
    if (m_dtype != DataType::INT64) {
         throw std::runtime_error("Attempting to get long long flat data from a non-INT64 tensor.");
    }
    return m_int_data;
#endif
}

template<>
std::vector<double> JTensor::get_flat<double>() const {
#ifdef TENSORFLOW_ENABLED
    if (m_dtype != DataType::FLOAT64) {
        throw std::runtime_error("Attempting to get double flat data from a non-FLOAT64 tensor.");
    }
    if (!m_has_tf_tensor) {
        return {};
    }
    auto flat_tensor = m_tf_tensor.flat<double>();
    return std::vector<double>(flat_tensor.data(), flat_tensor.data() + flat_tensor.size());
#else
    if (m_dtype != DataType::FLOAT64) {
        throw std::runtime_error("Attempting to get double flat data from a non-FLOAT64 tensor.");
    }
    return m_float_data;
#endif
}

// ===== JTensor Implementation =====

JTensor::JTensor() : m_dtype(DataType::UNKNOWN) {
#ifdef TENSORFLOW_ENABLED
    m_has_tf_tensor = false;
#endif
}

std::shared_ptr<JTensor> JTensor::zeros(const std::vector<long long>& shape) {
    auto tensor = std::shared_ptr<JTensor>(new JTensor());
    tensor->m_shape = shape;
    tensor->m_dtype = DataType::FLOAT64;
    
    // Initialize with zeros
    size_t total_size = 1;
    for (auto dim : shape) {
        total_size *= dim;
    }
    
#ifdef TENSORFLOW_ENABLED
    tensorflow::TensorShape tf_shape;
    for (auto dim : shape) {
        tf_shape.AddDim(dim);
    }
    tensor->m_tf_tensor = tensorflow::Tensor(tensorflow::DT_DOUBLE, tf_shape);
    auto flat = tensor->m_tf_tensor.flat<double>();
    for (int i = 0; i < flat.size(); ++i) {
        flat(i) = 0.0;
    }
    tensor->m_has_tf_tensor = true;
#else
    tensor->m_float_data.resize(total_size, 0.0);
#endif
    
    return tensor;
}

std::shared_ptr<JTensor> JTensor::scalar(double value) {
    auto tensor = std::shared_ptr<JTensor>(new JTensor());
    tensor->init_from_data(std::vector<double>{value}, {});
    return tensor;
}

std::shared_ptr<JTensor> JTensor::scalar(long long value) {
    auto tensor = std::shared_ptr<JTensor>(new JTensor());
    tensor->init_from_data(std::vector<long long>{value}, {});
    return tensor;
}

std::shared_ptr<JTensor> JTensor::from_data(const std::vector<double>& data, const std::vector<long long>& shape) {
    auto tensor = std::shared_ptr<JTensor>(new JTensor());
    tensor->init_from_data(data, shape);
    return tensor;
}

std::shared_ptr<JTensor> JTensor::from_data(const std::vector<long long>& data, const std::vector<long long>& shape) {
    auto tensor = std::shared_ptr<JTensor>(new JTensor());
    tensor->init_from_data(data, shape);
    return tensor;
}

std::shared_ptr<JTensor> JTensor::copy(const JTensor& other) {
    auto tensor = std::shared_ptr<JTensor>(new JTensor());
    tensor->m_shape = other.m_shape;
    tensor->m_dtype = other.m_dtype;
    
#ifdef TENSORFLOW_ENABLED
    if (other.m_has_tf_tensor) {
        // Deep copy the TensorFlow tensor
        tensor->m_tf_tensor = tensorflow::Tensor(other.m_tf_tensor.dtype(), other.m_tf_tensor.shape());
        tensor->m_tf_tensor.flat<double>() = other.m_tf_tensor.flat<double>();
        tensor->m_has_tf_tensor = true;
    } else {
        tensor->m_has_tf_tensor = false;
    }
#endif
    
    // Copy stub data
    tensor->m_float_data = other.m_float_data;
    tensor->m_int_data = other.m_int_data;
    tensor->m_string_data = other.m_string_data;
    
    return tensor;
}

#ifdef TENSORFLOW_ENABLED
JTensor::JTensor(tensorflow::Tensor tf_tensor) 
    : m_tf_tensor(std::move(tf_tensor)), m_has_tf_tensor(true) {
    // Extract shape
    auto tf_shape = m_tf_tensor.shape();
    for (int i = 0; i < tf_shape.dims(); ++i) {
        m_shape.push_back(tf_shape.dim_size(i));
    }
    
    // Determine data type
    switch (m_tf_tensor.dtype()) {
        case tensorflow::DT_DOUBLE:
        case tensorflow::DT_FLOAT:
            m_dtype = DataType::FLOAT64;
            break;
        case tensorflow::DT_INT64:
        case tensorflow::DT_INT32:
            m_dtype = DataType::INT64;
            break;
        case tensorflow::DT_STRING:
            m_dtype = DataType::STRING;
            break;
        default:
            m_dtype = DataType::UNKNOWN;
    }
}
#endif

JTensor::~JTensor() = default;

void JTensor::init_from_data(const std::vector<double>& data, const std::vector<long long>& shape) {
    m_dtype = DataType::FLOAT64;
    
    if (shape.empty()) {
        // Scalar or 1D array
        if (data.size() == 1) {
            m_shape = {}; // Scalar
        } else {
            m_shape = {static_cast<long long>(data.size())};
        }
    } else {
        m_shape = shape;
        size_t expected_size = 1;
        for (auto dim : shape) {
            expected_size *= dim;
        }
        assert(data.size() == expected_size && "Data size doesn't match shape");
    }
    
#ifdef TENSORFLOW_ENABLED
    tensorflow::TensorShape tf_shape;
    for (auto dim : m_shape) {
        tf_shape.AddDim(dim);
    }
    m_tf_tensor = tensorflow::Tensor(tensorflow::DT_DOUBLE, tf_shape);
    auto flat = m_tf_tensor.flat<double>();
    for (size_t i = 0; i < data.size(); ++i) {
        flat(i) = data[i];
    }
    m_has_tf_tensor = true;
#else
    m_float_data = data;
#endif
}

void JTensor::init_from_data(const std::vector<long long>& data, const std::vector<long long>& shape) {
    m_dtype = DataType::INT64;
    
    if (shape.empty()) {
        if (data.size() == 1) {
            m_shape = {}; // Scalar
        } else {
            m_shape = {static_cast<long long>(data.size())};
        }
    } else {
        m_shape = shape;
        size_t expected_size = 1;
        for (auto dim : shape) {
            expected_size *= dim;
        }
        assert(data.size() == expected_size && "Data size doesn't match shape");
    }
    
#ifdef TENSORFLOW_ENABLED
    tensorflow::TensorShape tf_shape;
    for (auto dim : m_shape) {
        tf_shape.AddDim(dim);
    }
    m_tf_tensor = tensorflow::Tensor(tensorflow::DT_INT64, tf_shape);
    auto flat = m_tf_tensor.flat<tensorflow::int64>();
    for (size_t i = 0; i < data.size(); ++i) {
        flat(i) = data[i];
    }
    m_has_tf_tensor = true;
#else
    m_int_data = data;
#endif
}

std::vector<long long> JTensor::shape() const {
    return m_shape;
}

size_t JTensor::rank() const {
    return m_shape.size();
}

size_t JTensor::size() const {
    return std::accumulate(m_shape.begin(), m_shape.end(), 1LL, std::multiplies<long long>());
}

template<>
double JTensor::get_scalar() const {
    assert(m_shape.empty() && "Not a scalar tensor");
    
#ifdef TENSORFLOW_ENABLED
    if (m_has_tf_tensor) {
        return m_tf_tensor.scalar<double>()();
    }
#endif
    
    if (m_dtype == DataType::FLOAT64 && !m_float_data.empty()) {
        return m_float_data[0];
    } else if (m_dtype == DataType::INT64 && !m_int_data.empty()) {
        return static_cast<double>(m_int_data[0]);
    }
    return 0.0;
}

template<>
long long JTensor::get_scalar() const {
    assert(m_shape.empty() && "Not a scalar tensor");
    
#ifdef TENSORFLOW_ENABLED
    if (m_has_tf_tensor) {
        if (m_tf_tensor.dtype() == tensorflow::DT_INT64) {
            return m_tf_tensor.scalar<tensorflow::int64>()();
        } else {
            return static_cast<long long>(m_tf_tensor.scalar<double>()());
        }
    }
#endif
    
    if (m_dtype == DataType::INT64 && !m_int_data.empty()) {
        return m_int_data[0];
    } else if (m_dtype == DataType::FLOAT64 && !m_float_data.empty()) {
        return static_cast<long long>(m_float_data[0]);
    }
    return 0;
}

JTensor::DataType JTensor::dtype() const {
    return m_dtype;
}

void JTensor::print(std::ostream& os) const {
    os << "JTensor(shape=[";
    for (size_t i = 0; i < m_shape.size(); ++i) {
        if (i > 0) os << ", ";
        os << m_shape[i];
    }
    os << "], dtype=";
    
    switch (m_dtype) {
        case DataType::FLOAT64:
            os << "FLOAT64";
            break;
        case DataType::INT64:
            os << "INT64";
            break;
        case DataType::STRING:
            os << "STRING";
            break;
        default:
            os << "UNKNOWN";
    }
    
    os << ", data=";
    
    if (m_shape.empty()) {
        // Scalar
        if (m_dtype == DataType::FLOAT64) {
            os << get_scalar<double>();
        } else if (m_dtype == DataType::INT64) {
            os << get_scalar<long long>();
        } else {
            os << "?";
        }
    } else if (m_shape.size() == 1 && m_shape[0] <= 10) {
        // Small 1D tensor - print all values
        os << "[";
        if (m_dtype == DataType::FLOAT64) {
            auto data = get_flat<double>();
            for (size_t i = 0; i < data.size(); ++i) {
                if (i > 0) os << ", ";
                os << data[i];
            }
        } else if (m_dtype == DataType::INT64) {
            auto data = get_flat<long long>();
            for (size_t i = 0; i < data.size(); ++i) {
                if (i > 0) os << ", ";
                os << data[i];
            }
        }
        os << "]";
    } else {
        // Larger tensor - just print shape
        os << "<tensor of size " << size() << ">";
    }
    
    os << ")";
}

std::shared_ptr<JTensor> from_data(const std::vector<long long>& data, const std::vector<long long>& shape) {
    return JTensor::from_data(data, shape);
}

// ===== TFSession Implementation =====

TFSession::TFSession() : m_initialized(false) {
#ifdef TENSORFLOW_ENABLED
    // Initialize TensorFlow session
    // In a real implementation, this would set up the session and load a graph
    // For now, just mark as initialized
    m_initialized = true;
#else
    m_initialized = true;  // Stub mode is always "initialized"
#endif
}

TFSession::~TFSession() = default;

bool TFSession::is_initialized() const {
    return m_initialized;
}

#ifdef TENSORFLOW_ENABLED
// TensorFlow operations
std::shared_ptr<JTensor> TFSession::add(const std::shared_ptr<JTensor>& a, const std::shared_ptr<JTensor>& b) {
    if (!a || !b) return nullptr;
    if (!m_initialized) return nullptr;
    
    // Check if shapes match
    if (a->shape() == b->shape()) {
        // Create result tensor with same shape
        tensorflow::Tensor tf_a = a->get_tf_tensor();
        tensorflow::Tensor tf_b = b->get_tf_tensor();
        tensorflow::Tensor result(tf_a.dtype(), tf_a.shape());
        
        if (tf_a.dtype() == tensorflow::DT_DOUBLE) {
            auto flat_a = tf_a.flat<double>();
            auto flat_b = tf_b.flat<double>();
            auto flat_result = result.flat<double>();
            
            for (int i = 0; i < flat_a.size(); ++i) {
                flat_result(i) = flat_a(i) + flat_b(i);
            }
        } else if (tf_a.dtype() == tensorflow::DT_INT64) {
            auto flat_a = tf_a.flat<tensorflow::int64>();
            auto flat_b = tf_b.flat<tensorflow::int64>();
            auto flat_result = result.flat<tensorflow::int64>();
            
            for (int i = 0; i < flat_a.size(); ++i) {
                flat_result(i) = flat_a(i) + flat_b(i);
            }
        }
        
        return std::make_shared<JTensor>(std::move(result));
    }
    
    return nullptr; // Shape mismatch
}
#else
// Stub implementation
std::shared_ptr<JTensor> TFSession::stub_binary_op(const std::shared_ptr<JTensor>& a, 
                                                  const std::shared_ptr<JTensor>& b,
                                                  const std::string& op_name) {
    if (!a || !b) return nullptr;
    
    // Simple stub: just return a copy of the first tensor
    std::cout << "Stub operation: " << op_name << " on tensors" << std::endl;
    return JTensor::copy(*a);
}

std::shared_ptr<JTensor> TFSession::add(const std::shared_ptr<JTensor>& a, const std::shared_ptr<JTensor>& b) {
    return stub_binary_op(a, b, "add");
}
#endif

#ifdef TENSORFLOW_ENABLED
std::shared_ptr<JTensor> TFSession::subtract(const std::shared_ptr<JTensor>& a, const std::shared_ptr<JTensor>& b) {
    // Similar to add but with subtraction
    // Implementation would be similar to add()
    return nullptr; // Placeholder
}
#else
std::shared_ptr<JTensor> TFSession::subtract(const std::shared_ptr<JTensor>& a, const std::shared_ptr<JTensor>& b) {
    return stub_binary_op(a, b, "subtract");
}
#endif

#ifdef TENSORFLOW_ENABLED
std::shared_ptr<JTensor> TFSession::multiply(const std::shared_ptr<JTensor>& a, const std::shared_ptr<JTensor>& b) {
    return nullptr; // Placeholder
}
#else
std::shared_ptr<JTensor> TFSession::multiply(const std::shared_ptr<JTensor>& a, const std::shared_ptr<JTensor>& b) {
    return stub_binary_op(a, b, "multiply");
}
#endif

#ifdef TENSORFLOW_ENABLED
std::shared_ptr<JTensor> TFSession::divide(const std::shared_ptr<JTensor>& a, const std::shared_ptr<JTensor>& b) {
    return nullptr; // Placeholder
}
#else
std::shared_ptr<JTensor> TFSession::divide(const std::shared_ptr<JTensor>& a, const std::shared_ptr<JTensor>& b) {
    return stub_binary_op(a, b, "divide");
}
#endif

std::shared_ptr<JTensor> TFSession::iota(long long n) {
    std::vector<long long> data;
    for (long long i = 0; i < n; ++i) {
        data.push_back(i);
    }
    return JTensor::from_data(data, std::vector<long long>{n});
}

std::shared_ptr<JTensor> TFSession::reshape(const std::shared_ptr<JTensor>& tensor, const std::vector<long long>& new_shape) {
    if (!tensor) return nullptr;
    
    // Verify that the total size matches
    size_t current_size = tensor->size();
    size_t new_size = 1;
    for (auto dim : new_shape) {
        new_size *= dim;
    }
    
    if (current_size != new_size) {
        std::cerr << "Reshape error: size mismatch" << std::endl;
        return nullptr;
    }
    
#ifdef TENSORFLOW_ENABLED
    // Implement TensorFlow reshape
    return nullptr; // Placeholder
#else
    // Stub: create new tensor with same data but different shape
    std::cout << "Stub reshape operation" << std::endl;
    auto result = JTensor::copy(*tensor);
    return result;
#endif
}

#ifdef TENSORFLOW_ENABLED
std::shared_ptr<JTensor> TFSession::transpose(const std::shared_ptr<JTensor>& tensor) {
    return nullptr; // Placeholder
}
#else
std::shared_ptr<JTensor> TFSession::transpose(const std::shared_ptr<JTensor>& tensor) {
    std::cout << "Stub transpose operation" << std::endl;
    return JTensor::copy(*tensor);
}
#endif

#ifdef TENSORFLOW_ENABLED
std::shared_ptr<JTensor> TFSession::reduce_sum(const std::shared_ptr<JTensor>& tensor, const std::vector<int>& axes) {
    return nullptr; // Placeholder
}
#else
std::shared_ptr<JTensor> TFSession::reduce_sum(const std::shared_ptr<JTensor>& tensor, const std::vector<int>& axes) {
    std::cout << "Stub reduce_sum operation" << std::endl;
    
    if (!tensor) return nullptr;
    
    // For now, implement a simple total sum (reduce over all axes)
    // In a full implementation, we'd handle the axes parameter properly
    
    if (tensor->dtype() == JTensor::DataType::INT64) {
        auto flat = tensor->get_flat<long long>();
        long long sum = 0;
        for (auto val : flat) {
            sum += val;
        }
        return JTensor::scalar(sum);
    } else if (tensor->dtype() == JTensor::DataType::FLOAT64) {
        auto flat = tensor->get_flat<double>();
        double sum = 0.0;
        for (auto val : flat) {
            sum += val;
        }
        return JTensor::scalar(sum);
    }
    
    return nullptr;
}
#endif

// Implementation for the JValue version of reduce_sum
JValue TFSession::reduce_sum(const JValue& operand) {
    // Check if the operand is a tensor
    if (!std::holds_alternative<std::shared_ptr<JTensor>>(operand)) {
        throw std::runtime_error("Operand for reduce_sum must be a JTensor.");
    }
    
    const auto& tensor_ptr = std::get<std::shared_ptr<JTensor>>(operand);
    if (!tensor_ptr) {
        throw std::runtime_error("Null JTensor provided to reduce_sum.");
    }

    // Use the tensor version and return the result
    auto result = reduce_sum(tensor_ptr);
    return result;
}

} // namespace JInterpreter
