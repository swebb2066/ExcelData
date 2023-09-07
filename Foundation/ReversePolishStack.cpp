#include <Foundation/ReversePolishStack.h>
#include <Foundation/Yaml.h>

namespace Foundation
{

    log4cxx::LoggerPtr
OperatorMap::m_log(log4cxx::Logger::getLogger("OperatorMap"));

// The code associated with the known operator \c name
    OperatorMap::OperatorCode
OperatorMap::GetOperatorCode(const StringType& name)
{
    typedef std::map<StringType, OperatorCode> OperatorStore;
    static OperatorStore m_knownOperators;
    if (m_knownOperators.empty())
    {
        m_knownOperators["abs"] = AbsoluteOperator;
        m_knownOperators["+"] = AddOperator;
        m_knownOperators["-"] = SubtractOperator;
        m_knownOperators["x"] = MultiplyOperator;
        m_knownOperators["*"] = MultiplyOperator;
        m_knownOperators["/"] = DivideOperator;
        m_knownOperators["^"] = PowerOperator;
        m_knownOperators["cos"] = CosineOperator;
        m_knownOperators["sin"] = SineOperator;
        m_knownOperators["tan"] = TanOperator;
        m_knownOperators["acos"] = InverseCosineOperator;
        m_knownOperators["asin"] = InverseSineOperator;
        m_knownOperators["atan"] = InverseTanOperator;
        m_knownOperators["acos2"] = InverseCosine2Operator;
        m_knownOperators["asin2"] = InverseSine2Operator;
        m_knownOperators["atan2"] = InverseTan2Operator;
        m_knownOperators["sqrt"] = SquareRootOperator;
        m_knownOperators["pi"] = PushPiOperator;
        m_knownOperators["dup"] = DupOperator;
        m_knownOperators["add2"] = Add2VectorOperator;
        m_knownOperators["sub2"] = Subtract2VectorOperator;
        m_knownOperators["mult2"] = Mult2VectorOperator;
        m_knownOperators["div2"] = Div2VectorOperator;
        m_knownOperators["norm2"] = Norm2VectorOperator;
        m_knownOperators["dot2"] = Dot2VectorOperator;
        m_knownOperators["perp2"] = Perp2VectorOperator;
        m_knownOperators["add3"] = Add3VectorOperator;
        m_knownOperators["sub3"] = Subtract3VectorOperator;
        m_knownOperators["mult3"] = Mult3VectorOperator;
        m_knownOperators["div3"] = Div3VectorOperator;
        m_knownOperators["norm3"] = Norm3VectorOperator;
        m_knownOperators["dot3"] = Dot3VectorOperator;
        m_knownOperators["cross3"] = Cross3VectorOperator;
    }
    OperatorStore::const_iterator pItem = m_knownOperators.find(name);
    return (pItem == m_knownOperators.end()) ? UnknownOperator : pItem->second;
}

// The stack size increase caused by processing \c name
    int
OperatorMap::GetStackSizeIncrement(const StringType& name)
{
    int result = 1;
    switch (GetOperatorCode(name))
    {
    case AbsoluteOperator:
        result = 0;
        break;
    case AddOperator:
    case SubtractOperator:
    case MultiplyOperator:
    case DivideOperator:
    case PowerOperator:
    case InverseCosine2Operator:
    case InverseSine2Operator:
    case InverseTan2Operator:
        result = -2;
        break;
    case CosineOperator:
    case SineOperator:
    case TanOperator:
    case InverseCosineOperator:
    case InverseSineOperator:
    case InverseTanOperator:
    case SquareRootOperator:
        result = -1;
        break;
    case PushPiOperator:
    case DupOperator:
        result = 1;
        break;
    case Add2VectorOperator:
    case Subtract2VectorOperator:
    case Mult2VectorOperator:
    case Div2VectorOperator:
        result = -3;
        break;
    case Norm2VectorOperator:
        result = -2;
        break;
    case Dot2VectorOperator:
    case Perp2VectorOperator:
        result = -4;
        break;
    case Add3VectorOperator:
    case Subtract3VectorOperator:
    case Mult3VectorOperator:
    case Div3VectorOperator:
        result = -4;
        break;
    case Norm3VectorOperator:
        result = -3;
        break;
    case Dot3VectorOperator:
        result = -6;
        break;
    case Cross3VectorOperator:
        result = -3;
        break;
    }
    LOG4CXX_DEBUG(m_log, "GetStackSizeIncrement: " << name << " = " << result);
    return result;
}

/// Is \c item referenced in \c path
    bool
OperatorMap::IsRecursiveReference(const YAML::Node& item, const NodeChain* path)
{
    bool result = false;
    if (path->seq == item)
        result = true;
    else if (path->parent)
        result = IsRecursiveReference(item, path->parent);
    return result;
}

// The stack size increase caused by processing \c seq
    int
OperatorMap::GetStackSizeIncrement(const YAML::Node& seq, const NodeChain* path)
{
    LOG4CXX_TRACE(m_log, "GetStackSizeIncrement:" << " node@ " << seq.Mark().pos);
    NodeChain current = { seq, path };
    int stackSize = 1;
    for ( YAML::Node::const_iterator item = seq.begin()
        ; seq.end() != item
        ; ++item)
    {
        if (item->IsScalar())
            stackSize += GetStackSizeIncrement(item->Scalar());
        else if (IsRecursiveReference(*item, &current))
            throw YAML::RecursiveDefinition(item->Mark());
        else
            stackSize += std::max(1, GetStackSizeIncrement(*item, &current));
        if (ReversePolishStack::MaxStackDepth < stackSize)
            throw YAML::StackTooDeep(item->Mark(), "Reverse polish", ReversePolishStack::MaxStackDepth);
    }
    LOG4CXX_DEBUG(m_log, "GetStackSizeIncrement: " << seq << " = " << stackSize);
    return stackSize;
}

// The code of the operator that consumes \c *term
    OperatorMap::OperatorCode
OperatorMap::GetOperatorCode(const YAML::Node::const_iterator& term, const YAML::Node::const_iterator& end)
{
    int stackSize = 1;
    YAML::Node::const_iterator item = term;
    StringType lastItem;
    while (++item != end && 0 < stackSize)
    {
        if (item->IsScalar())
        {
            lastItem = item->Scalar();
            stackSize += GetStackSizeIncrement(lastItem);
        }
        else
            stackSize += std::max(1, GetStackSizeIncrement(*item));
    }
    LOG4CXX_DEBUG(m_log, "GetOperatorCode: " << *term << " = " << lastItem);
    return GetOperatorCode(lastItem);
}

// Is \c item a term in a trigonometric function
    bool
OperatorMap::IsTrigonometricTerm(const YAML::Node::const_iterator& item, const YAML::Node::const_iterator& end, bool& inverse)
{
    switch (GetOperatorCode(item, end))
    {
    case CosineOperator:
    case SineOperator:
    case TanOperator:
        inverse = false;
        return true;
    case InverseCosineOperator:
    case InverseSineOperator:
    case InverseTanOperator:
    case InverseCosine2Operator:
    case InverseSine2Operator:
    case InverseTan2Operator:
        inverse = true;
        return true;
    }
    return false;
}

// Is \c item a term in a division or multiplication?
    bool
OperatorMap::IsUnitCombiningTerm(const YAML::Node::const_iterator& item, const YAML::Node::const_iterator& end)
{
    OperatorCode opCode = GetOperatorCode(item, end);
    return DivideOperator == opCode || MultiplyOperator == opCode;
}

    log4cxx::LoggerPtr
ReversePolishStack::m_log(log4cxx::Logger::getLogger("ReversePolishStack"));

    boost::weak_ptr<ValueSource>
ReversePolishStack::m_constantSource;

// A stack built from the nodes in \c seq
ReversePolishStack::ReversePolishStack(const YAML::Node& seq)
{
    for (YAML::Node::const_iterator item = seq.begin(); item != seq.end(); ++item)
    {
        if (!item->IsScalar() || !ApplyOperator(item->Scalar()))
            PushValue(item->as<RealType>());
    }
}

// The value at \c depth (in 1..Depth())
    ReversePolishStack::RealType
ReversePolishStack::ValueAt(size_t depth) const
{
    assert(0 < depth && depth <= Depth());
    return m_stack[depth - 1];
}

// The substring of \c name that excludes leading and trailing whitespace
    std::string
ReversePolishStack::WhiteStripped(const std::string& name)
{
    std::string result;
    static const char* whitespaces = " \t\f\v";
    size_t nonwhiteStart = name.find_first_not_of(whitespaces);
    if (nonwhiteStart != std::string::npos)
    {
        size_t nonwhiteEnd = name.find_last_not_of(whitespaces);
        result = name.substr(nonwhiteStart, nonwhiteEnd - nonwhiteStart + 1);
    }
    return result;
}

// Is \c name a known operator that has been applied?
    bool
ReversePolishStack::ApplyOperator(const StringType& name)
{
    bool result = false;
    OperatorCode op = GetOperatorCode(WhiteStripped(name));
    switch (op)
    {
    case AbsoluteOperator:
        if (!m_stack.empty())
        {
            RealType term = m_stack.back();
            m_stack.back() = std::abs(term);
            LOG4CXX_DEBUG(m_log, "abs(" << term << ")= " << m_stack.back());
            result = true;
        }
        break;
    case AddOperator:
        if (1 < m_stack.size())
        {
            RealType term1 = m_stack.back();
            m_stack.pop_back();
            RealType term2 = m_stack.back();
            m_stack.back() += term1;
            LOG4CXX_DEBUG(m_log, term1 << " + " << term2 << " = " << m_stack.back());
            result = true;
        }
        break;
    case SubtractOperator:
            result = true;
        if (1 < m_stack.size())
        {
            RealType term1 = m_stack.back();
            m_stack.pop_back();
            RealType term2 = m_stack.back();
            m_stack.back() = term1 - term2;
            LOG4CXX_DEBUG(m_log, term1 << " - " << term2 << " = " << m_stack.back());
            result = true;
        }
        else if (!m_stack.empty())
        {
            RealType term = m_stack.back();
            m_stack.back() = -term;
            result = true;
            LOG4CXX_DEBUG(m_log, "-" << term << " = " << m_stack.back());
        }
        break;
    case MultiplyOperator:
        if (1 < m_stack.size())
        {
            RealType term1 = m_stack.back();
            m_stack.pop_back();
            RealType term2 = m_stack.back();
            m_stack.back() *= term1;
            LOG4CXX_DEBUG(m_log, term1 << " * " << term2 << " = " << m_stack.back());
            result = true;
        }
        break;
    case DivideOperator:
        if (1 < m_stack.size())
        {
            RealType term1 = m_stack.back();
            m_stack.pop_back();
            RealType term2 = m_stack.back();
            m_stack.back() = term1 / term2;
            LOG4CXX_DEBUG(m_log, term1 << " / " << term2 << " = " << m_stack.back());
            result = true;
        }
        break;
    case PowerOperator:
        if (1 < m_stack.size())
        {
            RealType term1 = m_stack.back();
            m_stack.pop_back();
            RealType term2 = m_stack.back();
            m_stack.back() = std::pow(term1, term2);
            LOG4CXX_DEBUG(m_log, term1 << "^" << term2 << " = " << m_stack.back());
            result = true;
        }
        break;
    case CosineOperator:
        if (!m_stack.empty())
        {
            RealType term = m_stack.back();
            m_stack.back() = std::cos(term);
            LOG4CXX_DEBUG(m_log, "cos(" << term << ")= " << m_stack.back());
            result = true;
        }
        break;
    case SineOperator:
        if (!m_stack.empty())
        {
            RealType term = m_stack.back();
            m_stack.back() = std::sin(term);
            LOG4CXX_DEBUG(m_log, "sin(" << term << ")= " << m_stack.back());
            result = true;
        }
        break;
    case TanOperator:
        if (!m_stack.empty())
        {
            RealType term = m_stack.back();
            m_stack.back() = std::tan(term);
            LOG4CXX_DEBUG(m_log, "tan(" << term << ")= " << m_stack.back());
            result = true;
        }
        break;
    case InverseCosineOperator:
        if (!m_stack.empty())
        {
            RealType term = m_stack.back();
            m_stack.back() = std::acos(term);
            LOG4CXX_DEBUG(m_log, "acos(" << term << ")= " << m_stack.back());
            result = true;
        }
        break;
    case InverseSineOperator:
        if (!m_stack.empty())
        {
            RealType term = m_stack.back();
            m_stack.back() = std::asin(term);
            LOG4CXX_DEBUG(m_log, "asin(" << term << ")= " << m_stack.back());
            result = true;
        }
        break;
    case InverseTanOperator:
        if (!m_stack.empty())
        {
            RealType term = m_stack.back();
            m_stack.back() = std::atan(term);
            LOG4CXX_DEBUG(m_log, "atan(" << term << ")= " << m_stack.back());
            result = true;
        }
        break;
    case InverseCosine2Operator:
        if (1 < m_stack.size())
        {
            RealType term1 = m_stack.back();
            m_stack.pop_back();
            RealType term2 = m_stack.back();
            m_stack.back() = std::acos(term1 / term2);
            LOG4CXX_DEBUG(m_log, "acos2(" << term1 << ", " << term2 << ")= " << m_stack.back());
            result = true;
        }
        break;
    case InverseSine2Operator:
        if (1 < m_stack.size())
        {
            RealType term1 = m_stack.back();
            m_stack.pop_back();
            RealType term2 = m_stack.back();
            m_stack.back() = std::asin(term1 / term2);
            LOG4CXX_DEBUG(m_log, "asin2(" << term1 << ", " << term2 << ")= " << m_stack.back());
            result = true;
        }
        break;
    case InverseTan2Operator:
        if (1 < m_stack.size())
        {
            RealType term1 = m_stack.back();
            m_stack.pop_back();
            RealType term2 = m_stack.back();
            m_stack.back() = std::atan2(term1, term2);
            LOG4CXX_DEBUG(m_log, "atan2(" << term1 << ", " << term2 << ")= " << m_stack.back());
            result = true;
        }
        break;
    case SquareRootOperator:
        if (!m_stack.empty())
        {
            RealType term = m_stack.back();
            m_stack.back() = std::sqrt(term);
            LOG4CXX_DEBUG(m_log, "sqrt(" << term << ")= " << m_stack.back());
            result = true;
        }
        break;
    case PushPiOperator:
        m_stack.push_back(M_PI);
        result = true;
        break;
    case DupOperator:
        if (!m_stack.empty())
        {
            m_stack.push_back(m_stack.back());
            result = true;
        }
        break;
    case Subtract2VectorOperator:
    case Add2VectorOperator:
    case Mult2VectorOperator:
    case Div2VectorOperator:
    //case Perp2VectorOperator:
    //    if (3 < m_stack.size())
    //    {
    //        Vector<RealType, 2> term1;
    //        for (size_t i = 2; 0 < i; )
    //        {
    //            --i;
    //            term1[i] = m_stack.back();
    //            m_stack.pop_back();
    //        }
    //        Vector<RealType, 2> term2;
    //        for (size_t i = 2; 0 < i; )
    //        {
    //            --i;
    //            term2[i] = m_stack.back();
    //            m_stack.pop_back();
    //        }
    //        if (Perp2VectorOperator == op)
    //        {
    //            RealType perpProduct = term1.PerpProduct(term2);
    //            LOG4CXX_DEBUG(m_log, "perp2vec" << term1 << " x " << term2 << "= " << perpProduct);
    //            m_stack.push_back(perpProduct);
    //        }
    //        else for (size_t i = 0; i < 2; ++i)
    //        {
    //            if (Subtract2VectorOperator == op)
    //            {
    //                m_stack.push_back(term1[i] - term2[i]);
    //                LOG4CXX_DEBUG(m_log, "sub2v[" << i << "] " << term1[i] << " + " << term2[i] << "= " << m_stack.back());
    //            }
    //            else if (Add2VectorOperator == op)
    //            {
    //                m_stack.push_back(term1[i] + term2[i]);
    //                LOG4CXX_DEBUG(m_log, "add2v[" << i << "] " << term1[i] << " + " << term2[i] << "= " << m_stack.back());
    //            }
    //            else if (Mult2VectorOperator == op)
    //            {
    //                m_stack.push_back(term1[i] * term2[i]);
    //                LOG4CXX_DEBUG(m_log, "mult2v[" << i << "] " << term1[i] << " * " << term2[i] << "= " << m_stack.back());
    //            }
    //            else if (Div2VectorOperator == op)
    //            {
    //                m_stack.push_back(term1[i] / term2[i]);
    //                LOG4CXX_DEBUG(m_log, "div2v[" << i << "] " << term1[i] << " / " << term2[i] << "= " << m_stack.back());
    //            }
    //        }
    //        result = true;
    //    }
    //    break;
    //case Dot2VectorOperator:
    //    if (1 < m_stack.size())
    //    {
    //        Vector<RealType, 2> term1;
    //        for (size_t i = 2; 0 < i; )
    //        {
    //            --i;
    //            term1[i] = m_stack.back();
    //            m_stack.pop_back();
    //        }
    //        Vector<RealType, 2> term2;
    //        for (size_t i = 2; 0 < i; )
    //        {
    //            --i;
    //            term2[i] = m_stack.back();
    //            m_stack.pop_back();
    //        }
    //        m_stack.push_back(term1.DotProduct(term2));
    //        LOG4CXX_DEBUG(m_log, "dot2vec" << term1 << " . " << term2 << "= " << m_stack.back());
    //        result = true;
    //    }
    //    break;
    //case Norm2VectorOperator:
    //    if (1 < m_stack.size())
    //    {
    //        Vector<RealType, 2> term1;
    //        for (size_t i = 0; i < 2; ++i)
    //        {
    //            term1[i] = m_stack.back();
    //            m_stack.pop_back();
    //        }
    //        m_stack.push_back(term1.Len());
    //        LOG4CXX_DEBUG(m_log, "norm2v" << "= " << m_stack.back());
    //        result = true;
    //    }
    //    break;
    //case Subtract3VectorOperator:
    //case Add3VectorOperator:
    //case Mult3VectorOperator:
    //case Div3VectorOperator:
    //case Cross3VectorOperator:
    //    if (5 < m_stack.size())
    //    {
    //        Vector<RealType, 3> term1;
    //        for (size_t i = 3; 0 < i; )
    //        {
    //            --i;
    //            term1[i] = m_stack.back();
    //            m_stack.pop_back();
    //        }
    //        Vector<RealType, 3> term2;
    //        for (size_t i = 3; 0 < i; )
    //        {
    //            --i;
    //            term2[i] = m_stack.back();
    //            m_stack.pop_back();
    //        }
    //        if (Cross3VectorOperator == op)
    //        {
    //            Vector<RealType, 3> crossProduct = term1.CrossProduct(term2);
    //            LOG4CXX_DEBUG(m_log, "cross3vec" << term1 << " x " << term2 << "= " << crossProduct);
    //            for (size_t i = 0; i < 3; ++i)
    //                m_stack.push_back(crossProduct[i]);
    //        }
    //        else for (size_t i = 0; i < 3; ++i)
    //        {
    //            if (Subtract3VectorOperator == op)
    //            {
    //                m_stack.push_back(term1[i] - term2[i]);
    //                LOG4CXX_DEBUG(m_log, "sub3v[" << i << "] " << term1[i] << " + " << term2[i] << "= " << m_stack.back());
    //            }
    //            else if (Add3VectorOperator == op)
    //            {
    //                m_stack.push_back(term1[i] + term2[i]);
    //                LOG4CXX_DEBUG(m_log, "add3v[" << i << "] " << term1[i] << " + " << term2[i] << "= " << m_stack.back());
    //            }
    //            else if (Mult3VectorOperator == op)
    //            {
    //                m_stack.push_back(term1[i] * term2[i]);
    //                LOG4CXX_DEBUG(m_log, "mult3v[" << i << "] " << term1[i] << " * " << term2[i] << "= " << m_stack.back());
    //            }
    //            else if (Div3VectorOperator == op)
    //            {
    //                m_stack.push_back(term1[i] / term2[i]);
    //                LOG4CXX_DEBUG(m_log, "div3v[" << i << "] " << term1[i] << " / " << term2[i] << "= " << m_stack.back());
    //            }
    //        }
    //        result = true;
    //    }
    //    break;
    //case Dot3VectorOperator:
    //    if (2 < m_stack.size())
    //    {
    //        Vector<RealType, 3> term1;
    //        for (size_t i = 3; 0 < i; )
    //        {
    //            --i;
    //            term1[i] = m_stack.back();
    //            m_stack.pop_back();
    //        }
    //        Vector<RealType, 3> term2;
    //        for (size_t i = 3; 0 < i; )
    //        {
    //            --i;
    //            term2[i] = m_stack.back();
    //            m_stack.pop_back();
    //        }
    //        m_stack.push_back(term1.DotProduct(term2));
    //        LOG4CXX_DEBUG(m_log, "dot3vec" << term1 << " . " << term2 << "= " << m_stack.back());
    //        result = true;
    //    }
    //    break;
    //case Norm3VectorOperator:
    //    if (2 < m_stack.size())
    //    {
    //        Vector<RealType, 3> term1;
    //        for (size_t i = 0; i < 3; ++i)
    //        {
    //            term1[i] = m_stack.back();
    //            m_stack.pop_back();
    //        }
    //        m_stack.push_back(term1.Len());
    //        LOG4CXX_DEBUG(m_log, "norm3v" << "= " << m_stack.back());
    //        result = true;
    //    }
        break;
    }
    return result;
}

// The constant value of the constant \c name
    bool
ReversePolishStack::GetConstantValue(const StringType& name, StringType& result) const
{
    bool found = false;
    ValueSourcePtr pSource = m_constantSource.lock();
    if (pSource)
        found = pSource->GetValue(WhiteStripped(name), result);
    LOG4CXX_TRACE(m_log, "GetConstantValue: " << name << " found? " << found);
    return found;
}

// Add \c value to the m_stack
    void
ReversePolishStack::PushValue(const RealType& value)
{
    LOG4CXX_TRACE(m_log, "PushValue: " << value);
    m_stack.push_back(value);
}

// The sum of al values on the m_stack
    ReversePolishStack::RealType
ReversePolishStack::PopAll()
{
    RealType value(0);
    while (!m_stack.empty())
    {
        value += m_stack.back();
        m_stack.pop_back();
    }
    return value;
}

// Extend this stack with the elements of \c other
    void
ReversePolishStack::PushStack(const ThisType& other)
{
    m_stack.insert(m_stack.end(), other.m_stack.begin(), other.m_stack.end());
}

} // namespace Foundation
