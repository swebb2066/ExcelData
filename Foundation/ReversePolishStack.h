#ifndef FOUNDATION_REVERSE_POLISH_STACK_HDR
#define FOUNDATION_REVERSE_POLISH_STACK_HDR
#include <Foundation/Logger.h>
#include <Foundation/ValueSource.h>
#include <Foundation/Utility.h>
#include <yaml-cpp/yaml.h>
#include <string>
#include <map>
#include <vector>
#include <cmath>
#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>

namespace Foundation
{

/// A map from a string name to an operator code
class OperatorMap
{
public: // Types
    typedef double RealType;
    typedef std::string StringType;

protected: // Types
    enum OperatorCode
    { UnknownOperator = 0
    , AbsoluteOperator = 'a'
    , AddOperator = '+'
    , SubtractOperator = '-'
    , MultiplyOperator = '*'
    , DivideOperator = '/'
    , PowerOperator = '^'
    , CosineOperator = 'c'
    , SineOperator = 's'
    , TanOperator = 't'
    , InverseCosineOperator = 'C'
    , InverseSineOperator = 'S'
    , InverseTanOperator = 'T'
    , InverseCosine2Operator = 'C' + ('i' << 8)
    , InverseSine2Operator = 'S' + ('i' << 8)
    , InverseTan2Operator = 'T' + ('i' << 8)
    , SquareRootOperator = 'r'
    , PushPiOperator = 'p'
    , DupOperator = '='
    , Add2VectorOperator = '+' + ('2' << 8) + ('v' << 16)
    , Subtract2VectorOperator = '-' + ('2' << 8) + ('v' << 16)
    , Mult2VectorOperator = '*' + ('2' << 8) + ('v' << 16)
    , Div2VectorOperator = '/' + ('2' << 8) + ('v' << 16)
    , Norm2VectorOperator = 'n' + ('2' << 8) + ('v' << 16)
    , Dot2VectorOperator = '.' + ('2' << 8) + ('v' << 16)
    , Perp2VectorOperator = 'x' + ('2' << 8) + ('v' << 16)
    , Add3VectorOperator = '+' + ('3' << 8) + ('v' << 16)
    , Subtract3VectorOperator = '-' + ('3' << 8) + ('v' << 16)
    , Mult3VectorOperator = '*' + ('3' << 8) + ('v' << 16)
    , Div3VectorOperator = '/' + ('3' << 8) + ('v' << 16)
    , Norm3VectorOperator = 'n' + ('3' << 8) + ('v' << 16)
    , Dot3VectorOperator = '.' + ('3' << 8) + ('v' << 16)
    , Cross3VectorOperator = 'x' + ('3' << 8) + ('v' << 16)
    };
    /// For recusive reference checking
    struct NodeChain
    {
        const YAML::Node& seq;
        const NodeChain* parent;
    };

public: // Class methods
    /// The code associated with the known operator \c name
    static OperatorCode GetOperatorCode(const StringType& name);

    /// The stack size increase caused by processing \c name
    static int GetStackSizeIncrement(const StringType& name);

    /// The stack size increase caused by processing \c seq
    static int GetStackSizeIncrement(const YAML::Node& seq, const NodeChain* path = 0);

    /// Is \c item referenced in \c path
    static bool IsRecursiveReference(const YAML::Node& item, const NodeChain* path);

public: // YAML sequence methods
    /// The code of the operator that consumes \c *term
    OperatorCode GetOperatorCode(const YAML::Node::const_iterator& term, const YAML::Node::const_iterator& end);

    /// Is \c term a parameter in a division or multiplication?
    bool IsUnitCombiningTerm(const YAML::Node::const_iterator& term, const YAML::Node::const_iterator& end);

    /// Is \c term a parameter in a trigonometric function
    bool IsTrigonometricTerm(const YAML::Node::const_iterator& term, const YAML::Node::const_iterator& end, bool& inverse);

protected: // Class attributes
    static log4cxx::LoggerPtr m_log;
    static std::map<StringType, RealType> m_constantValue;
};

/// Apply known operators to a stack of values following reverse polish rules.
class ReversePolishStack : protected OperatorMap
{
    typedef ReversePolishStack ThisType;
protected: // Types
    typedef std::vector<RealType> StoreType;

protected: // Attributes
    StoreType m_stack;

public: // ...structors
    /// An empty stack
    ReversePolishStack() {}

    /// A stack built from the nodes in \c seq
    ReversePolishStack(const YAML::Node& seq);

public: // Accessors
    /// The value of the constant \c name
    virtual bool GetConstantValue(const StringType& name, StringType& result) const;

    /// The value at \c depth (in 1..Depth())
    RealType ValueAt(size_t depth) const;

    /// The number of available values
    size_t Depth() const { return m_stack.size(); }

public: // Modifiers
    /// Is \c name a known operator that has been applied?
    bool ApplyOperator(const StringType& name);

    /// Add \c value to the m_stack
    void PushValue(const RealType& value);

    /// The sum of al values on the m_stack
    RealType PopAll();

    /// Extend this stack with the elements of \c other
    void PushStack(const ThisType& other);

public: // Class methods
    typedef boost::shared_ptr<ValueSource> ValueSourcePtr;
    /// Set the provider of constant values
    static void PutValueSource(const ValueSourcePtr& source) { m_constantSource = source; }
    /// The maximum allowed statck depth
    static const int MaxStackDepth = 100;
    /// The substring of \c name that excludes leading and trailing whitespace
    static std::string WhiteStripped(const std::string& name);

protected: // Class attributes
    static boost::weak_ptr<ValueSource> m_constantSource;
    static log4cxx::LoggerPtr m_log;
};

} // namespace Foundation

#endif // FOUNDATION_REVERSE_POLISH_STACK_HDR
