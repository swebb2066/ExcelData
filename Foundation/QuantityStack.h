#ifndef FOUNDATION_QUANTITY_STACK_HDR
#define FOUNDATION_QUANTITY_STACK_HDR
#include <Foundation/ReversePolishStack.h>
#include <Foundation/QuantitySource.h>
#include <Foundation/Utility.h>
#include <Foundation/Yaml.h>
#define BOOST_TYPEOF_SILENT
#include <boost/units/quantity.hpp>
#include <boost/units/systems/angle/degrees.hpp>
#include <boost/units/systems/si/area.hpp>
#include <boost/units/systems/si/length.hpp>
#include <boost/units/systems/si/time.hpp>
#include <boost/type_traits/is_same.hpp>

#ifdef _MSC_VER
#pragma warning (disable : 4305) // double to float warning
#pragma warning (disable : 4244) // double to float warning
#endif

namespace Foundation
{

/// A reverse polish m_stack with a boost::units::quantity value.
    template <typename T, class D>
class ReversePolishQuantityStack : public ReversePolishStack
{
     typedef ReversePolishStack BaseType;
protected: // Types
    typedef T RealType;
    typedef typename boost::units::quantity<D, RealType> QuantityType;
    typedef typename boost::units::quantity<boost::units::si::area, RealType> AreaType;
    typedef typename boost::units::quantity<boost::units::si::time, RealType> DurationType;
    typedef typename boost::units::quantity<boost::units::si::length, RealType> LengthType;
    typedef typename boost::units::quantity<boost::units::degree::plane_angle, RealType> PlaneAngleType;
    typedef typename boost::units::quantity<boost::units::si::dimensionless, RealType> PortionType;

public: // ...structors
    /// An empty stack
    ReversePolishQuantityStack() { }

    /// A stack built from the nodes in \c seq
    ReversePolishQuantityStack(const YAML::Node& seq) { Push(seq); }

    /// For derived classes
    virtual ~ReversePolishQuantityStack() { }

protected: // Modifiers
    /// Extend this stack with the nodes in \c seq
    void Push(const YAML::Node& seq);

    /// Add the length value in the scalar \c node to the this stack. PRE: node.IsScalar()
    void PushLength(const YAML::Node& node);

    /// Add the plane angle value in the scalar \c node to the this stack. PRE: node.IsScalar()
    void PushPlaneAngle(const YAML::Node& node);

    /// Add the quantity in the scalar \c node to the this stack. PRE: node.IsScalar()
    bool PushQuantity(const YAML::Node& node);

public: // Accessors
    /// Extract the sum of the values from this stack
    virtual QuantityType GetResult()
    {
        QuantityType result = QuantityType::from_value(this->PopAll());
        LOG4CXX_TRACE(this->m_log, "GetResult: " << result);
        return result;
    }

    /// The value at \c depth (in 1..Depth())
    QuantityType ValueAt(size_t depth) const
    {
        return QuantityType::from_value(BaseType::ValueAt(depth));
    }

    /// The constant value of the constant \c name
    template <typename MT, class MD>
    bool GetQuantity(const StringType& name, boost::units::quantity<MD,MT>& result) const
    {
        bool found = false;
        auto pSource = m_constantSource.lock();
        if (auto pQuantitySource = dynamic_cast<QuantitySource<MT>*>(pSource.get()))
            found = pQuantitySource->GetQuantity(WhiteStripped(name), result);
        LOG4CXX_TRACE(m_log, "GetQuantity: " << name << " found? " << found);
        return found;
    }

public: // Modifiers
    /// Extend this stack with \c quantity
    virtual void PushQuantity(const QuantityType& quantity)
    {
        LOG4CXX_TRACE(this->m_log, "PushQuantity: " << quantity);
        this->PushValue(quantity.value());
    }
};

/// A reverse polish m_stack with a boost::units::quantity value.
    template <typename T, class D>
class QuantityStack : public ReversePolishQuantityStack<T, D>
{
    typedef ReversePolishQuantityStack<T, D> BaseType;
public: // ...structors
    /// A stack built from the nodes in \c seq
   QuantityStack(const YAML::Node& seq) { BaseType::Push(seq); }
};

/// A reverse polish m_stack with plane_angle quantity values in degrees.
    template <typename T>
class QuantityStack<T, boost::units::degree::plane_angle> : public ReversePolishQuantityStack<T, boost::units::degree::plane_angle>
{
    typedef ReversePolishQuantityStack<T, boost::units::degree::plane_angle> BaseType;
public: // Types
    typedef typename BaseType::RealType RealType;
    typedef typename BaseType::QuantityType QuantityType;

public: // ...structors
    /// A stack built from the nodes in \c seq
    QuantityStack(const YAML::Node& seq) { BaseType::Push(seq); }

public: // Accessors
    /// Extract the sum of the values from this stack
    QuantityType GetResult()
    {
        QuantityType result = QuantityType::from_value(this->PopAll() * M_DEGREES_PER_RADIAN);
        LOG4CXX_TRACE(this->m_log, "GetResult: " << result);
        return result;
    }

    /// The value at \c depth (in 1..Depth())
    QuantityType ValueAt(size_t depth) const
    {
        return QuantityType::from_value(ReversePolishStack::ValueAt(depth) * M_DEGREES_PER_RADIAN);
    }

public: // Modifiers
    /// Extend this stack with \c quantity
    void PushQuantity(const QuantityType& quantity)
    {
        LOG4CXX_TRACE(this->m_log, "PushQuantity: " << quantity);
        this->PushValue(quantity.value() * M_RADIANS_PER_DEGREE);
    }

};

// Add the length quantity in \c node to the this stack. PRE: node.IsScalar()
    template <typename T, class D>
    void
ReversePolishQuantityStack<T,D>::PushLength(const YAML::Node& node)
{
    LOG4CXX_DEBUG(this->m_log, "PushLength: " << node.Scalar());
    RealType real;
    LengthType quantity;
    StringType stringValue;
    if (YAML::convert<LengthType>::decode(node, quantity))
        this->PushValue(quantity.value());
    else if (YAML::convert<RealType>::decode(node, real))
        this->PushValue(real);
    else if (this->GetQuantity(node.Scalar(), quantity))
        this->PushValue(quantity.value());
    else if (this->GetConstantValue(node.Scalar(), stringValue))
        this->PushLength(YAML::Load(stringValue));
    else
        throw YAML::RequiredItemType(node.Mark(), "length", node.Scalar());
}

// Add the plane angle quantity in \c node to the this stack. PRE: node.IsScalar()
    template <typename T, class D>
    void
ReversePolishQuantityStack<T,D>::PushPlaneAngle(const YAML::Node& node)
{
    LOG4CXX_DEBUG(this->m_log, "PushPlaneAngle: " << node.Scalar());
    RealType real;
    PlaneAngleType quantity;
    StringType stringValue;
    if (YAML::convert<PlaneAngleType>::decode(node, quantity))
        this->PushValue(quantity.value() * M_RADIANS_PER_DEGREE);
    else if (YAML::convert<RealType>::decode(node, real))
        this->PushValue(real);
    else if (this->GetQuantity(node.Scalar(), quantity))
        this->PushValue(quantity.value() * M_RADIANS_PER_DEGREE);
    else if (this->GetConstantValue(node.Scalar(), stringValue))
        this->PushPlaneAngle(YAML::Load(stringValue));
    else
        throw YAML::RequiredItemType(node.Mark(), "angle", node.Scalar());
}

// Add the quantity in \c node to the this stack. PRE: node.IsScalar()
    template <typename T, class D>
    bool
ReversePolishQuantityStack<T,D>::PushQuantity(const YAML::Node& node)
{
    LOG4CXX_DEBUG(this->m_log, "PushQuantity: " << node.Scalar());
    bool result = true;
    RealType real;
    QuantityType quantity;
    StringType stringValue;
    if (YAML::convert<QuantityType>::decode(node, quantity))
        this->PushQuantity(quantity);
    else if (YAML::convert<RealType>::decode(node, real))
        this->PushValue(real);
    else if (this->GetQuantity(node.Scalar(), quantity))
        this->PushQuantity(quantity);
    else if (this->GetConstantValue(node.Scalar(), stringValue))
        result = this->PushQuantity(YAML::Load(stringValue));
    else
        result = false;
    return result;
}

// Extend this stack with the nodes in \c seq
    template <typename T, class D>
    void
ReversePolishQuantityStack<T,D>::Push(const YAML::Node& seq)
{
    for (YAML::Node::const_iterator item = seq.begin(); item != seq.end(); ++item)
    {
        bool isInverse;
        if (this->IsTrigonometricTerm(item, seq.end(), isInverse))
        {
            LOG4CXX_DEBUG(this->m_log, "TrigonometricTerm:" << " inverse? " << isInverse);
            if (item->IsSequence())
            {
                if (isInverse)
                    PushStack(QuantityStack<T,boost::units::si::length>(*item));
                else
                    PushStack(QuantityStack<T,boost::units::degree::plane_angle>(*item));
            }
            else if (!item->IsScalar())
                throw YAML::RequiredItemType(item->Mark(), "scalar", "push");
            else if (!this->ApplyOperator(item->Scalar()))
            {
                if (isInverse)
                    this->PushLength(*item);
                else
                    this->PushPlaneAngle(*item);
            }
        }
        else if (item->IsSequence())
            this->PushStack(QuantityStack<T,D>(*item));
        else if (!item->IsScalar())
            throw YAML::RequiredItemType(item->Mark(), "scalar", "push");
        else if (this->ApplyOperator(item->Scalar()))
            ;
        else if (this->PushQuantity(*item))
            ;
        else if (boost::is_same<D, boost::units::si::length>::value)
        {
            PlaneAngleType quantity;
            if (!this->IsUnitCombiningTerm(item, seq.end()) || !YAML::convert<PlaneAngleType>::decode(*item, quantity))
                throw YAML::RequiredItemType(item->Mark(), "length", item->Scalar());
            else
                this->PushValue(quantity.value() * M_RADIANS_PER_DEGREE);
        }
        else if (boost::is_same<D, boost::units::si::area>::value)
        {
            AreaType quantity;
            if (!this->IsUnitCombiningTerm(item, seq.end()) || !YAML::convert<AreaType>::decode(*item, quantity))
                throw YAML::RequiredItemType(item->Mark(), "area", item->Scalar());
            else
                this->PushValue(quantity.value());
        }
        else if (boost::is_same<D, boost::units::si::time>::value)
        {
            DurationType quantity;
            if (!this->IsUnitCombiningTerm(item, seq.end()) || !YAML::convert<DurationType>::decode(*item, quantity))
                throw YAML::RequiredItemType(item->Mark(), "time", item->Scalar());
            else
                this->PushValue(quantity.value());
        }
        else if (boost::is_same<D, boost::units::degree::plane_angle>::value)
        {
            LengthType quantity;
            if (!this->IsUnitCombiningTerm(item, seq.end()) || !YAML::convert<LengthType>::decode(*item, quantity))
                throw YAML::RequiredItemType(item->Mark(), "angle", item->Scalar());
            else
                this->PushValue(quantity.value());
        }
        else if (boost::is_same<D, boost::units::si::dimensionless>::value)
        {
            PortionType portion;
            PlaneAngleType angle;
            LengthType length;
            if (DivideOperator != this->GetOperatorCode(item, seq.end()))
                throw YAML::RequiredItemType(item->Mark(), "portion", item->Scalar());
            else if (YAML::convert<PlaneAngleType>::decode(*item, angle))
                this->PushValue(angle.value());
            else if (YAML::convert<LengthType>::decode(*item, length))
                this->PushValue(length.value());
            else if (YAML::convert<PortionType>::decode(*item, portion))
                this->PushValue(portion.value());
            else
                throw YAML::RequiredItemType(item->Mark(), "value", item->Scalar());
        }
        else
            throw YAML::InvalidValue(item->Mark(), item->Scalar(), "calculation");
    }
}

} // namespace Foundation

#endif // FOUNDATION_QUANTITY_STACK_HDR
