// Extensions to the YAML namespace
#include <Foundation/Yaml.h>
#include <Foundation/YamlDocument.h>
#include <Foundation/Exception.h>
#include <Foundation/StdLogger.h>
#ifdef YAML_EVENT_HANDLER_PROVIDES_NODE_EXTENT
#include <yaml-cpp/NodeBuilder.h>
#endif // YAML_EVENT_HANDLER_PROVIDES_NODE_EXTENT
#include <fstream>

namespace YAML
{

// The name corresponding to \c type
    std::string
to_string(NodeType::value type)
{
    switch (type)
    {
    default:
    case NodeType::Undefined:
        break;
    case NodeType::Null:
        return "Null";
    case NodeType::Scalar:
        return "Scalar";
    case NodeType::Sequence:
        return "Sequence";
    case NodeType::Map:
        return "Map";
    }
    return "Undefined";
}

// The index between \c startIndex and \c endIndex of the first start of line
    static size_t
NewLineIndex(const std::string& data, size_t startIndex, size_t endIndex)
{
    size_t result;
    size_t newlineIndex = data.find("\n", startIndex);
    if (newlineIndex == data.npos || endIndex < newlineIndex)
        result = endIndex;
    else
        result = newlineIndex + 1;
    return result;
}

class NodeTypeException : public Exception
{
public: // ...stuctors
    NodeTypeException(const Mark& context, NodeType::value type)
        : Exception(context, "unexpected " + to_string(type) + " node type")
    {   }
};

class InvalidNodePosition : public Exception
{
public: // ...stuctors
    InvalidNodePosition(const Mark& context, NodeType::value type)
        : Exception(context, "not a " + to_string(type))
    {}
};

#ifdef YAML_EVENT_HANDLER_PROVIDES_NODE_EXTENT
class Document::Builder : public NodeBuilder
{
    Document* m_parent; //!<
public: // ...structors
    Builder(Document* parent) : m_parent(parent) {}

public: // Hooked methods
    void OnNodeEnd(NodeType::value type, const Mark& startMark, const Mark& endMark);
};
#endif

// A copy without the shallowest level index. Precondition: HasInternalIndex()
    Document::UpdateMark
Document::UpdateMark::InternalUpdateMark() const
{
    UpdateMark result(*this);
    result.updatePath.pop_back();
    return result;
}

// A node with locations which refer to the base document (not an update to the base document)
Document::UpdateNode::UpdateNode(const Node& data)
    : yaml(data)
{}

// A emitted change to the base document
Document::UpdateNode::UpdateNode(const StringPtr& text)
    : newText(text)
{}

// A node in an emitted change at \c path in the base document
Document::UpdateNode::UpdateNode(const Node& data, const std::vector<size_t>& path)
    : yaml(data)
    , updatePath(path)
{}

// Use \c emittedText as the emitted change to the base document.
    void
Document::UpdateNode::ChangeTextTo(const std::string& emittedText)
{
    if (newText)
        *newText = emittedText;
    yaml = Node();
}

// The re-parsed version of the emitted change
    const Node&
Document::UpdateNode::Data() const
{
    if (yaml.IsNull() && this->newText)
        const_cast<UpdateNode*>(this)->yaml = YAML::Load(*this->newText);
    return yaml;
}

// A key to the emitted change
    Document::UpdateMark
Document::UpdateNode::Mark() const
{
    return UpdateMark(this->Data().Mark(), this->updatePath.begin(), this->updatePath.end());
}

// The number of elements in the emitted change
    size_t
Document::UpdateNode::size() const
{
    return Data().size();
}

// Put a textual version of this update onto \c os
    void
Document::UpdateNode::Write(std::ostream& os) const
{
    os << updatePath;
    if (this->newText)
        os << '\n' << *this->newText;
    else if (this->yaml)
        os << '\n' << this->yaml;
}

// The name corresponding to \c type
    std::string
Document::ToName(EditType type)
{
    switch (type)
    {
    default:
        break;
    case Append:
        return "add";
    case Insert:
        return "insert";
    case Modify:
        return "update";
    }
    return "???";
}

    log4cxx::LoggerPtr
Document::m_log(log4cxx::Logger::getLogger("YamlDocument"));

// An empty document with a \c rootType at the top level
Document::Document(const NodeType::value& rootType)
    : m_rootType(rootType)
{
    if (NodeType::Undefined != rootType)
    {
        NodePosition key = {Mark(), rootType};
        m_nodeIndex[key] = StartEndDataStore(1, std::make_pair(Mark(), Mark()));
    }
}

// A map document with \c subDoc as value for \c key
Document::Document(const StringType& mapKey, const Document& subDoc)
    : m_rootType(NodeType::Map)
    , m_content(new StringType(mapKey + ":\n  ~"))
    , m_updateStore(1, subDoc)
{
    NodePosition nodeIndexKey = {Mark(), m_rootType};
    m_nodeIndex[nodeIndexKey] = StartEndDataStore(1, std::make_pair(Mark(), Mark()));
    auto root = ParseContent();
    NodePosition nullKey{root.begin()->second.Mark(), NodeType::Null, 0};
    auto itemRange = GetItemsAt(nullKey);
    auto indent = itemRange[0].first.column;
    size_t updateIndex = 0;
    auto resumeAt = itemRange[0].second;
    m_updateIndex[nullKey] = UpdateData{ Modify, indent, updateIndex, resumeAt };
}

// Change this into an empty document
    void
Document::Reset()
{
    LOG4CXX_TRACE(m_log, "Reset:");
    m_content.reset();
    m_updateIndex.clear();
    m_updateStore.clear();
    m_nodeIndex.clear();
    if (NodeType::Undefined != m_rootType)
    {
        NodePosition key = {Mark(), m_rootType};
        m_nodeIndex[key] = StartEndDataStore(1, std::make_pair(Mark(), Mark()));
    }
}

// Change this into an empty document
    void
Document::ResetUpdates()
{
    LOG4CXX_TRACE(m_log, "ResetUpdates:");
    m_updateStore.clear();
    m_updateIndex.clear();
}

// Set this index from \c content
    Node
Document::Load(const StringType& content)
{
    LOG4CXX_DEBUG(m_log, "Load:\n" << content);
    Reset();
    m_content.reset(new StringType(content));
    return m_content->empty() ? Node() : ParseContent();
}

// Set \c m_content from \c input
    Node
Document::Load(std::istream& input)
{
    Reset();
    m_content.reset(new StringType());
    // Copy the text
    StringType line;
    while (std::getline(input, line))
    {
        if (3 <= line.size() &&
            0 == line.compare(0, 3, "...") ||
            0 == line.compare(0, 3, "---") )
            break;
        m_content->append(line);
        m_content->append("\n");
    }
    return m_content->empty() ? Node() : ParseContent();
}

// Set \c m_content from \c fileName
    Node
Document::LoadFile(const PathType& fileName)
{
    LOG4CXX_DEBUG(m_log, "LoadFile: " << fileName);
    if (!exists(fileName))
    {
        Foundation::MissingFileException ex(fileName);
        LOG4CXX_WARN(m_log, ex.what() << " in LoadFile");
        throw ex;
    }
    std::ifstream fin(fileName.c_str());
    return Load(fin);
}

// Write this content to \c os. Precondition: !IsEmpty()
    void
Document::Store(std::ostream& os) const
{
    Mark outMark;
    for (StartToUpdateMap::const_iterator it = m_updateIndex.begin(); it != m_updateIndex.end(); ++it)
    {
        const EditType& editType = it->second.type;
        const Mark& copyToMark = it->first.mark;
        if (outMark.pos < copyToMark.pos)
        {
            StringType value = m_content->substr(outMark.pos, copyToMark.pos - outMark.pos);
            LOG4CXX_TRACE(m_log, "Store: " << "copy " << outMark.pos << " to " << copyToMark.pos << " value:\n" << value);
            os << value;
        }
        std::stringstream ss;
        m_updateStore[it->second.docIndex].Store(ss);
        StringType value = ss.str();
        LOG4CXX_TRACE(m_log, "Store: " << ToName(editType) << " at " << copyToMark << " value:\n" << value);
        size_t lineLength = 0;
        if (0 <= it->second.indent)
        {
            StringType indent(it->second.indent, ' ');
            for (size_t startIndex = 0; startIndex < value.size(); startIndex += lineLength)
            {
                size_t nextIndex = NewLineIndex(value, startIndex, value.size());
                lineLength = nextIndex - startIndex;
                if (0 < startIndex)
                    os << indent;
                else if (editType == Append)
                    os << "\n" << indent;
                os << value.substr(startIndex, lineLength);
            }
            if (editType == Insert)
                os << "\n" << indent;
        }
        else if (editType == Insert)
            os << value << ", ";
        else if (editType == Append)
            os << ", " << value;
        else
            os << value;
        outMark = it->second.resumeAt;
    }
    if (0 <= outMark.pos && m_content)
    {
        StringType value = m_content->substr(outMark.pos);
        LOG4CXX_TRACE(m_log, "Store: " << "copyFrom " << outMark.pos << " value:\n" << value);
        os << value;
    }
}

// Write this content to \c fileName
    void
Document::StoreFile(const PathType& fileName) const
{
    LOG4CXX_DEBUG(m_log, "StoreFile: " << fileName);
    std::ofstream fout(fileName.c_str());
    Store(fout);
    fout.flush();
    if (fout.bad())
    {
        Foundation::WriteFileException ex(fileName);
        LOG4CXX_WARN(m_log, ex.what() << " in StoreFile");
        throw ex;
    }
}

// Add \c newVal at the end of the \c type node at \c location.
    Document::UpdateNode
Document::AppendTo(const NodeType::value& type, const UpdateMark& location, const StringType& newVal)
{
    LOG4CXX_DEBUG(m_log, "Append: type " << type << " updatePath " << location.updatePath);
    UpdateNode result;
    if (location.HasInternalIndex())
    {
        result = m_updateStore[location.InternalIndex()].AppendTo(type, location.InternalUpdateMark(), newVal);
        result.updatePath.push_back(location.InternalIndex());
    }
    else
    {
        NodePosition key = {location, type, 0};
        result = this->Update(key, newVal, Append);
    }
    return result;
}

// Add \c newVal at the end of the map node at \c location.
    Document::UpdateNode
Document::AppendToMap(const Emitter& newVal, const UpdateMark& location)
{
    LOG4CXX_DEBUG(m_log, "AppendToMap: updatePath " << location.updatePath);
    return AppendTo(NodeType::Map, location, newVal.c_str());
}

// Add \c newVal at the end of the sequence node at \c location.
    Document::UpdateNode
Document::AppendToSequence(const Emitter& newVal, const UpdateMark& location)
{
    LOG4CXX_DEBUG(m_log, "AppendToSequence: updatePath " << location.updatePath);
    return AppendTo(NodeType::Sequence, location, newVal.c_str());
}

// Add \c newVal before \c atIndex of the \c type node at \c location.
    Document::UpdateNode
Document::InsertIn(const StringType& newVal, const NodeType::value& type, const UpdateMark& location, size_t atItem)
{
    LOG4CXX_DEBUG(m_log, "Insert: type " << type << " updatePath " << location.updatePath);
    UpdateNode result;
    if (location.HasInternalIndex())
    {
        result = m_updateStore[location.InternalIndex()].InsertIn(newVal, type, location.InternalUpdateMark(), atItem);
        result.updatePath.push_back(location.InternalIndex());
    }
    else
    {
        NodePosition key = {location, type, 0};
        result = this->Update(key, newVal.c_str(), Insert, atItem);
    }
    return result;
}

// Add \c newVal before \c atItem of the map node at \c location.
    Document::UpdateNode
Document::InsertInMap(const Emitter& newVal, const UpdateMark& location, size_t atItem)
{
    LOG4CXX_DEBUG(m_log, "InsertInMap: updatePath " << location.updatePath);
    return InsertIn(newVal.c_str(), NodeType::Map, location, atItem);
}

// Add \c newVal before \c atItem of the sequence node at \c location.
    Document::UpdateNode
Document::InsertInSequence(const Emitter& newVal, const UpdateMark& location, size_t atItem)
{
    LOG4CXX_DEBUG(m_log, "InsertInSequence: updatePath " << location.updatePath);
    return InsertIn(newVal.c_str(), NodeType::Sequence, location, atItem);
}

// Merge the map elements in \c newVal into the map node at \c location.
    Document::UpdateNode
Document::MergeIntoMap(const Emitter& newVal, const UpdateMark& location)
{
    StringStore oldItems = GetMapItems(location);
    UpdateNode result = UpdateMap(location, newVal);
    StringType newText = result.GetNewText();
    LOG4CXX_DEBUG(m_log, "MergeIntoMap: newText\n" << newText);
    bool changed = false;
    for ( StringStore::const_iterator pItem = oldItems.begin()
        ; oldItems.end() != pItem
        ; ++pItem)
    {
        StringType key = pItem->substr(0, pItem->find(':'));
        if (newText.npos == newText.find(key + ':'))
        {
            LOG4CXX_DEBUG(m_log, "MergeIntoMap: add " << key);
            newText.append('\n' + *pItem);
            changed = true;
        }
    }
    if (changed)
        result.ChangeTextTo(newText);
    return result;
}

// Remove the \c type item at \c location
    Document::UpdateNode
Document::Remove(const NodeType::value& type, const UpdateMark& location)
{
    LOG4CXX_DEBUG(m_log, "Remove:" << " type " << type << " updatePath " << location.updatePath);
    UpdateNode result;
    if (location.HasInternalIndex())
    {
        result = m_updateStore[location.InternalIndex()].Remove(type, location.InternalUpdateMark());
        result.updatePath.push_back(location.InternalIndex());
    }
    else
    {
        NodePosition key = {location, type, 0};
        result = this->Update(key, StringType(), Modify);
    }
    return result;
}

// Remove the \c node change
    bool
Document::RemoveUpdate(const UpdateNode& node)
{
    LOG4CXX_DEBUG(m_log, "RemoveUpdate:" << " updatePath " << node.updatePath);
    bool result = false;
    auto targetDoc = this;
    auto entryNumber = node.updatePath.size();
    while (1 < entryNumber)
    {
        --entryNumber;
        targetDoc = &m_updateStore[entryNumber];
    }
    if (0 < entryNumber)
        result = targetDoc->RemoveUpdate(node.updatePath[entryNumber - 1]);
    return result;
}

// Remove the \c docIndex change
    bool
Document::RemoveUpdate(size_t docIndex)
{
    LOG4CXX_DEBUG(m_log, "RemoveUpdate:" << " docIndex " << docIndex << '/' << m_updateStore.size());
    bool result = false;
    for (auto item : m_updateIndex)
    {
        if (item.second.docIndex == docIndex)
        {
            LOG4CXX_DEBUG(m_log, "RemoveUpdate:"
                << " pos " << item.first.mark.pos
                << " type " << item.first.type
                << " additionPos " << item.first.additionPos
                );
            m_updateIndex.erase(item.first);
            result = true;
            break;
        }
    }
    return result;
}

// Remove the map at \c location
    Document::UpdateNode
Document::RemoveMap(const UpdateMark& location)
{
    LOG4CXX_DEBUG(m_log, "RemoveMap: updatePath " << location.updatePath);
    return Remove(NodeType::Map, location);
}

// Remove the scalar at \c location
    Document::UpdateNode
Document::RemoveScalar(const UpdateMark& location)
{
    LOG4CXX_DEBUG(m_log, "RemoveScalar: updatePath " << location.updatePath);
    return Remove(NodeType::Scalar, location);
}

// Remove the sequence at \c location
    Document::UpdateNode
Document::RemoveSequence(const UpdateMark& location)
{
    LOG4CXX_DEBUG(m_log, "RemoveSequence: updatePath " << location.updatePath);
    return Remove(NodeType::Sequence, location);
}

// Make \c newVal the value at \c location
    Document::UpdateNode
Document::Update(const NodeType::value& type, const UpdateMark& location, const StringType& newVal)
{
    LOG4CXX_DEBUG(m_log, "Update: type " << type << " updatePath " << location.updatePath);
    UpdateNode result;
    if (location.HasInternalIndex())
    {
        result = m_updateStore[location.InternalIndex()].Update(type, location.InternalUpdateMark(), newVal);
        result.updatePath.push_back(location.InternalIndex());
    }
    else
    {
        NodePosition key = {location, type, 0};
        result = this->Update(key, newVal, Modify);
    }
    return result;
}

// Change the map at \c location to \c newVal
    Document::UpdateNode
Document::UpdateMap(const UpdateMark& location, const Emitter& newVal)
{
    LOG4CXX_DEBUG(m_log, "UpdateMap: updatePath " << location.updatePath);
    return Update(NodeType::Map, location, newVal.c_str());
}

// Change the null at \c location to \c newVal
    Document::UpdateNode
Document::UpdateNull(const UpdateMark& location, const Emitter& newVal)
{
    LOG4CXX_DEBUG(m_log, "UpdateNull: updatePath " << location.updatePath);
    return Update(NodeType::Null, location, newVal.c_str());
}

// Change the scalar at \c location to \c newVal
    Document::UpdateNode
Document::UpdateScalar(const UpdateMark& location, const Emitter& newVal)
{
    LOG4CXX_DEBUG(m_log, "UpdateScalar: updatePath " << location.updatePath);
    return Update(NodeType::Scalar, location, newVal.c_str());
}

// Change the sequence at \c location to \c newVal
    Document::UpdateNode
Document::UpdateSequence(const UpdateMark& location, const Emitter& newVal)
{
    LOG4CXX_DEBUG(m_log, "UpdateSequence: updatePath " << location.updatePath);
    return Update(NodeType::Sequence, location, newVal.c_str());
}

// Change the entry at \c location to \c newVal
    Document::UpdateNode
Document::Update(const NodePosition& location, const StringType& newVal, EditType editType, size_t atIndex)
{
    LOG4CXX_DEBUG(m_log, "Update:"
        << ' ' << ToName(editType)
        << " type " << location.type
        << " at " << location.mark
        );
    const StartEndDataStore& itemRange = GetItemsAt(location);
    // Use insertion position in m_updateIndex
    Mark startMark = itemRange[atIndex].first;
    Mark endMark = itemRange[atIndex].second;
    if (editType == Insert && 0 < atIndex && newVal[0] == '-')
        while (0 < startMark.column)
        {
            --startMark.pos;
            --startMark.column;
            if (m_content->at(startMark.pos) == '-')
                break;
        }
    LOG4CXX_TRACE(m_log, "Update:"
        << " startMark " << startMark
        << " endMark " << endMark
        );
    Mark copyTo = (editType == Append) ? endMark : startMark;
    NodePosition key = { copyTo, location.type, 0 };
    size_t updateIndex = m_updateStore.size();
    StartToUpdateMap::iterator update = m_updateIndex.find(key);
    StringType docText(newVal);
    int indent = startMark.column;
    if (update == m_updateIndex.end())
        ;
    else if (editType == Modify)
    {
        updateIndex = update->second.docIndex;
    }
    else if (editType == Append)
    {
        for (key.additionPos = 1; m_updateIndex.find(key) != m_updateIndex.end(); ++key.additionPos)
            ;
    }
    else if (editType == Insert)
    {
        for (key.additionPos = -1; m_updateIndex.find(key) != m_updateIndex.end(); --key.additionPos)
            ;
    }
    Mark resumeAt = (editType == Modify) ? endMark : copyTo;
    if (!m_content) // First insert/append?
        m_content.reset(new StringType());
    else if (int(m_content->size()) <= startMark.pos)
        ;
    else if (NodeType::Sequence == location.type && m_content->at(startMark.pos) == '[') // A flow sequence?
    {
        indent = -1;
        if (2 < docText.size() && docText[0] == '-' && docText[1] == ' ') // Conversion to flow required?
            docText = docText.substr(2);
        if (editType == Modify && docText.empty()) // Remove sequence?
            docText = "~";
        else if (editType == Append)
        {
            if (m_content->at(startMark.pos + 1) == ']') // Adding to an empty list
            {
                editType = Modify; // Do not add ','
                key.mark = startMark;
                // Skip '['
                ++key.mark.pos;
                ++key.mark.column;
            }
            // Move insertion point into the flow?
            while (0 < resumeAt.column)
            {
                --resumeAt.pos;
                --resumeAt.column;
                if (resumeAt.column < key.mark.column)
                {
                    --key.mark.pos;
                    --key.mark.column;
                }
                if (m_content->at(resumeAt.pos) == ']')
                    break;
            }
        }
        else if (editType == Insert)
        {
            if (m_content->at(startMark.pos + 1) == ']') // Adding to an empty list
                editType = Modify; // Do not add ','
            // Skip '['
            ++key.mark.pos;
            ++key.mark.column;
            ++resumeAt.pos;
            ++resumeAt.column;
        }
    }
    else if (NodeType::Map == location.type && m_content->at(startMark.pos) == '{') // A flow map?
    {
        indent = -1;
        if (editType == Modify && docText.empty()) // Remove map?
            docText = "~";
    }
    else if (editType == Modify && docText.empty()) // Remove item?
        docText = "~";
    if (updateIndex == m_updateStore.size())
        m_updateStore.push_back(Document());
    LOG4CXX_TRACE(m_log, "Update:"
        << " docIndex " << updateIndex << '/' << m_updateStore.size()
        << " indent " << indent
        << " at " << key.mark.pos
        << " resumeAt " << resumeAt.pos
        << "\noldText:\n" << m_content->substr(key.mark.pos, resumeAt.pos - key.mark.pos)
        << "\nnewText:\n" << docText
        );
    UpdateData value = { editType, indent, updateIndex, resumeAt };
    if (key.mark.pos < resumeAt.pos) // Modifications to discard?
    {
        StartToUpdateMap::iterator discardItem = m_updateIndex.lower_bound(key);
        while (m_updateIndex.end() != discardItem &&
            (discardItem->first.mark.pos < resumeAt.pos ||
            (discardItem->first.mark.pos == resumeAt.pos && discardItem->first.type == location.type)))
        {
            LOG4CXX_TRACE(m_log, "Update:"
                << " discard type " << discardItem->second.type
                << " at " << discardItem->first.mark.pos
                << " to " << discardItem->second.resumeAt.pos
                );
            m_updateIndex.erase(discardItem);
            discardItem = m_updateIndex.lower_bound(key);
        }
    }
    m_updateIndex[key] = value;
    StringPtr sharedText(new StringType(docText));
    m_updateStore[updateIndex].m_content = sharedText;
    UpdateNode result(sharedText);
    result.updatePath.push_back(updateIndex);
    return result;
}

// The (unindented) text of the \c type value at \c location
    Document::StringType
Document::GetItem(const NodeType::value& type, const UpdateMark& location) const
{
    LOG4CXX_DEBUG(m_log, "GetItem:"
        << " type " << type
        << " at " << Mark(location)
        );
    StringType result;
    if (location.HasInternalIndex())
        result = m_updateStore[location.InternalIndex()].GetItem(type, location.InternalUpdateMark());
    else
    {
        // Get the range of the top level element
        NodePosition key = {location, type, 0};
        const StartEndData& startEnd = GetItemsAt(key)[0];
        result = GetUnindentedItemAt(startEnd);
    }
    LOG4CXX_DEBUG(m_log, "GetItem: result\n" << result);
    return result;
}

// The number of items in the \c type node at \c location
    size_t
Document::GetItemCount(const NodeType::value& type, const UpdateMark& location) const
{
    LOG4CXX_DEBUG(m_log, "GetItemCount:"
        << " type " << type
        << " at " << Mark(location)
        );
    size_t result;
    if (location.HasInternalIndex())
        result = m_updateStore[location.InternalIndex()].GetItemCount(type, location.InternalUpdateMark());
    else
    {
        NodePosition key = {location, type, 0};
        result = GetItemsAt(key).size() - 1;
    }
    return result;
}

// The (unindented) text of the items at \c location
    Document::StringStore
Document::GetItems(const NodeType::value& type, const UpdateMark& location) const
{
    LOG4CXX_DEBUG(m_log, "GetItems:"
        << " type " << type
        << " at " << Mark(location)
        );
    StringStore result;
    if (location.HasInternalIndex())
        result = m_updateStore[location.InternalIndex()].GetItems(type, location.InternalUpdateMark());
    else
    {
        NodePosition key = {location, type, 0};
        const StartEndDataStore& seqData = GetItemsAt(key);
        StartEndDataStore::const_iterator pItem = seqData.begin();
        for (++pItem; seqData.end() != pItem; ++pItem)
            result.push_back(GetUnindentedItemAt(*pItem));
    }
    return result;
}

// The end position of the \c type text at \c location
    Mark
Document::GetItemEnd(const NodeType::value& type, const UpdateMark& location) const
{
    LOG4CXX_DEBUG(m_log, "GetItemEnd:"
        << " type " << type
        << " at " << Mark(location)
        );
    Mark result = Mark::null_mark();
    if (location.HasInternalIndex())
        result = m_updateStore[location.InternalIndex()].GetItemEnd(type, location.InternalUpdateMark());
    else
    {
        NodePosition key = {location, type, 0};
        result = GetItemsAt(key)[0].second;
    }
    return result;
}

// The number of items in the map node at \c location
    size_t
Document::GetMapItemCount(const UpdateMark& location) const
{
    return GetItemCount(NodeType::Map, location);
}

// The (unindented) text of the items at \c location
    Document::StringStore
Document::GetMapItems(const UpdateMark& location) const
{
    return GetItems(NodeType::Map, location);
}

// The number of items in the sequence node at \c location
    size_t
Document::GetSequenceItemCount(const UpdateMark& location) const
{
    return GetItemCount(NodeType::Sequence, location);
}

// The (unindented) text of the items at \c location
    Document::StringStore
Document::GetSequenceItems(const UpdateMark& location) const
{
    return GetItems(NodeType::Sequence, location);
}

// The Yaml items starting at \c location
    const Document::StartEndDataStore&
Document::GetItemsAt(const NodePosition& location) const
{
    if (m_nodeIndex.size() <= 1 && m_content && !m_content->empty())
        const_cast<Document*>(this)->SetNodeIndex(YAML::Load(*m_content));
    NodeTextMap::const_iterator node;
    if (location.mark.pos <= 0)
    {
        node = m_nodeIndex.begin();
        if (m_nodeIndex.end() != node)
        {
            NodePosition firstItem = { node->first.mark, location.type, 0 };
            node = m_nodeIndex.find(firstItem);
        }
    }
    else
        node = m_nodeIndex.find(location);
    if (m_nodeIndex.end() == node)
    {
        InvalidNodePosition ex(location.mark, location.type);
        LOG4CXX_WARN(m_log, ex.what());
        throw ex;
    }
    LOG4CXX_DEBUG(m_log, "GetItemsAt: " << location.mark << " count " << node->second.size());
    return node->second;
}

// The text at \c startEnd with any indent removed
    Document::StringType
Document::GetUnindentedItemAt(const StartEndData& startEnd) const
{
    LOG4CXX_DEBUG(m_log, "GetUnindentedItemAt: " << startEnd);
    StringType result;
    // Calculate the indent size by finding the start of the line
    size_t index = startEnd.first.pos;
    while (0 < index && '\n' != m_content->at(--index))
        ;
    if (0 < index)
        ++index;
    size_t indentSize = startEnd.first.pos - index;

    // Remove the indent from each line of the return text
    std::stringstream ss(m_content->substr(startEnd.first.pos, startEnd.second.pos - startEnd.first.pos));
    char buf[MAX_LINE_SIZE+1];
    if (ss.getline(buf, MAX_LINE_SIZE))
        result.append(buf);
    while (ss.getline(buf, MAX_LINE_SIZE))
    {
        result.append("\n");
        size_t len = strlen(buf);
        if (indentSize < len)
            result.append(&buf[indentSize], len - indentSize);
    }
    LOG4CXX_DEBUG(m_log, "GetUnindentedItemAt: " << "result\n" << result);
    return result;
}

// Is \c location in \c m_nodeIndex?
    bool
Document::HasItemsAt(const NodePosition& location) const
{
    if (m_nodeIndex.size() <= 1 && m_content && !m_content->empty())
        const_cast<Document*>(this)->SetNodeIndex(YAML::Load(*m_content));
    NodeTextMap::const_iterator node;
    if (location.mark.pos <= 0)
    {
        node = m_nodeIndex.begin();
        if (m_nodeIndex.end() != node)
        {
            NodePosition firstItem = { node->first.mark, location.type, 0 };
            node = m_nodeIndex.find(firstItem);
        }
    }
    else
        node = m_nodeIndex.find(location);
    return node != m_nodeIndex.end();
}

// Is there a \c type item at \c location?
    bool
Document::HasTypeAt(const NodeType::value& type, const UpdateMark& location) const
{
    bool result;
    if (location.HasInternalIndex())
        result = m_updateStore[location.InternalIndex()].HasTypeAt(type, location.InternalUpdateMark());
    else
    {
        NodePosition key = {location, type, 0};
        result = HasItemsAt(key);
    }
    LOG4CXX_TRACE(m_log, "Has" << to_string(type) << "At@ " << location.pos << "? " << result);
    return result;
}

// The location of the first character of an anchor starting at \c mark
    Mark
Document::AnchorStart(const Mark& mark) const
{
    Mark result = mark;
    size_t anchorIndex = m_content->find_first_of("!&*\n", mark.pos);
    char startCh = 0;
    if (anchorIndex != m_content->npos)
        startCh = m_content->at(anchorIndex);
    if (startCh == '!' || startCh == '&' || startCh == '*')
    {
        result.pos = int(anchorIndex);
        result.column += result.pos - mark.pos;
    }
    return result;
}

// The location of the first character after any anchor (if any) starting at \c mark. Precondition: 0 <= mark.pos
    Mark
Document::AnchorEnd(const Mark& mark) const
{
    Mark endAnchor = mark;
    char startCh = 0;
    while ((size_t)endAnchor.pos < m_content->size() &&
        (startCh = m_content->at(endAnchor.pos)) == ' ')
        ++endAnchor.pos, ++endAnchor.column;
    if (startCh != '!' && startCh != '&' && startCh != '*')
        return mark;
    if (startCh == '!') // A tag?
    {
        size_t nonAnchor = m_content->find_first_of(" \n", endAnchor.pos);
        int nextPos = int(nonAnchor == m_content->npos ? m_content->size() : nonAnchor);
        endAnchor.column += nextPos - endAnchor.pos;
        endAnchor.pos = nextPos;
        while ((size_t)endAnchor.pos < m_content->size() && m_content->at(endAnchor.pos) == ' ')
            ++endAnchor.pos, ++endAnchor.column;
        if (m_content->size() <= (size_t)endAnchor.pos)
            return endAnchor;
        startCh = m_content->at(endAnchor.pos);
    }
    if (startCh != '&' && startCh != '*')
        return endAnchor;
    size_t nonAnchor = m_content->find_first_of(" ,:-{}[]\n", endAnchor.pos);
    int nextPos = int(nonAnchor == m_content->npos ? m_content->size() : nonAnchor);
    endAnchor.column += nextPos - endAnchor.pos;
    endAnchor.pos = nextPos;
    return endAnchor;
}

// The location of the first character after null starting at \c mark. Precondition: 0 <= mark.pos
    Mark
Document::NullEnd(const Mark& mark) const
{
    Mark endNull = mark;
    while ((size_t)endNull.pos < m_content->size() && m_content->at(endNull.pos) == ' ')
        ++endNull.pos, ++endNull.column;
    size_t nonNull = m_content->npos;
    if (endNull.pos != m_content->npos)
        nonNull = m_content->find_first_of(" ,:-{}[]\n", endNull.pos);
    int nextPos = int(nonNull == m_content->npos ? m_content->size() : nonNull);
    endNull.column += nextPos - endNull.pos;
    endNull.pos = nextPos;
    return endNull;
}

// The anchor name at \c location
    Document::StringType
Document::GetAnchorName(const Mark& location) const
{
    StringType result;
    if (!m_content)
        ;
    else if (0 <= location.pos && (size_t)location.pos < m_content->size())
    {
        size_t startIndex = location.pos;
        while (m_content->at(startIndex) == ' ')
            ++startIndex;
        LOG4CXX_TRACE(m_log, "GetAnchorName: " << m_content->substr(startIndex, m_content->find_first_of(" :-{}[]\n", startIndex)));
        if (m_content->at(startIndex) == '!')
        {
            startIndex = m_content->find_first_of(" \n", startIndex);
            while (m_content->at(startIndex) == ' ')
                ++startIndex;
            LOG4CXX_TRACE(m_log, "GetAnchorName: " << m_content->substr(startIndex, m_content->find_first_of(" :-{}[]\n", startIndex)));
        }
        if (m_content->at(startIndex) == '&' || m_content->at(startIndex) == '*')
        {
            ++startIndex;
            size_t endIndex = m_content->find_first_of(" :-{}[]\n", startIndex);
            result = m_content->substr(startIndex, endIndex - startIndex);
        }
    }
    LOG4CXX_DEBUG(m_log, "GetAnchorName@ " << location.pos << " result " << result);
    return result;
}

// The tag name at \c valueNode
    Document::StringType
Document::GetTagName( const Node& valueNode) const
{
    StringType result;
    result = valueNode.Tag();
    if (!result.empty() && '!' == result.front())
        result.erase(result.begin());
    if (result.empty())
        result = GetTagName(valueNode.Mark());
    return result;
}

// The tag name at \c location
    Document::StringType
Document::GetTagName(const Mark& location) const
{
    StringType result;
    if (!m_content)
        ;
    else if (0 <= location.pos && (size_t)location.pos < m_content->size())
    {
        size_t startIndex = location.pos;
        while (m_content->at(startIndex) == ' ')
            ++startIndex;
        LOG4CXX_TRACE(m_log, "GetTagName: " << m_content->substr(startIndex, m_content->find_first_of(" :-{}[]\n", startIndex)));
        if (startIndex < m_content->size() && m_content->at(startIndex) == '!') // A tag?
        {
            ++startIndex;
            if (startIndex < m_content->size() && m_content->at(startIndex) == '!') // A global tag? (e.g. !!foo )
                ++startIndex;
            size_t endIndex = startIndex;
            if (startIndex < m_content->size() && m_content->at(startIndex) == '<') // A verbatim tag? (e.g. !<!foo> )
            {
                ++startIndex;
                if (startIndex < m_content->size() && m_content->at(startIndex) == '!')
                {
                    ++startIndex;
                    endIndex = m_content->find_first_of(">\n", startIndex);
                }
            }
            else
                endIndex = m_content->find_first_of(" \n", startIndex);
            result = m_content->substr(startIndex, endIndex - startIndex);
        }
    }
    LOG4CXX_DEBUG(m_log, "GetTagName@ " << location.pos << " result " << result);
    return result;
}

#ifdef YAML_EVENT_HANDLER_PROVIDES_NODE_EXTENT
// Add a \c type node from \c startMark to \c endMark to \c m_nodeIndex
    void
Document::Builder::OnNodeEnd(NodeType::value type, const Mark& startMark, const Mark& endMark)
{
    LOG4CXX_TRACE(m_log, "OnNodeEnd: type " << type << " start " << startMark << " end " << endMark);
    NodePosition key = {startMark, type};
    m_parent->m_nodeIndex[key] = StartEndDataStore(1, std::make_pair(startMark, endMark));
}
#endif // !YAML_EVENT_HANDLER_PROVIDES_NODE_EXTENT

// The parsed representation of \c m_content
    Node
Document::ParseContent()
{
    std::stringstream input(*m_content, std::ios_base::in);
#ifdef YAML_EVENT_HANDLER_PROVIDES_NODE_EXTENT
    YAML::Parser parser(input);
    Builder builder(this);
    if(!parser.HandleNextDocument(builder))
        return Node();
    return builder.Root();
#else // !YAML_EVENT_HANDLER_PROVIDES_NODE_EXTENT
    Node result = YAML::Load(input);
    if (result)
        SetNodeIndex(result);
    return result;
#endif // YAML_EVENT_HANDLER_PROVIDES_NODE_EXTENT
}

class DuplicateKey : public Exception
{
public: // ...stuctors
    DuplicateKey(const Mark& mark, const std::string& item)
        : Exception(mark, "Duplicate map key (" + item + ')')
    {   }
};

// Set \c m_nodeIndex from \c root
    Mark
Document::SetNodeIndex(const Node& root, const StringType& indent)
{
    Mark rootLocation = root.Mark();
    NodePosition key = {rootLocation, root.Type()};
    Mark startMark = AnchorEnd(key.mark);
    while ((size_t)startMark.pos < m_content->size() && m_content->at(startMark.pos) == ' ')
        ++startMark.pos, ++startMark.column;
    Mark endMark = startMark;
    StartEndDataStore posData(1, std::make_pair(startMark, endMark));
    switch (root.Type())
    {
        default:
        case NodeType::Undefined:
        {
            NodeTypeException ex(root.Mark(), root.Type());
            LOG4CXX_WARN(m_log, ex.what() << " in SetNodeIndex");
            throw ex;
        }
        case NodeType::Null:
            endMark = NullEnd(endMark);
            break;
        case NodeType::Scalar:
        {
            const std::string& value = root.Scalar();
            // Find the location in m_content of the start of the scalar
            size_t startPos = m_content->find(value.c_str(), startMark.pos, NewLineIndex(value, 0, value.size()));
            if (startPos != m_content->npos)
            {
                int nextPos = int(startPos);
                startMark.column += nextPos - startMark.pos;
                startMark.pos = nextPos;
            }
            // Find the location in m_content of the end of the scalar
            endMark = startMark;
            int lineLength = 0;
            for (size_t startIndex = 0; startIndex < value.size(); startIndex += lineLength)
            {
                size_t nextIndex = NewLineIndex(value, startIndex, value.size());
                if (value[nextIndex - 1] == '\n')
                    ++endMark.line;
                lineLength = int(nextIndex - startIndex);
                if (0 < startIndex) // Account for indent
                    endMark.pos += startMark.column;
                endMark.pos += lineLength;
            }
            endMark.column = startMark.column + lineLength;
            LOG4CXX_TRACE(m_log, indent << key.mark << '"' << value << '"' << endMark);
            break;
        }
        case NodeType::Sequence:
        {
            StringType itemIndent = indent + "  ";
            LOG4CXX_TRACE(m_log, indent << key.mark << "BeginSeq");
            for (Node::const_iterator item = root.begin(); item != root.end(); ++item)
            {
                Mark startMark = item->Mark();
                if (rootLocation.pos < startMark.pos)
                    endMark = SetNodeIndex(*item, itemIndent);
                else // An alias
                {
                    startMark = AnchorStart(endMark);
                    endMark = AnchorEnd(startMark);
                    LOG4CXX_TRACE(m_log, itemIndent << "Alias " << startMark << " to " << endMark);
                }
                posData.push_back(std::make_pair(startMark, endMark));
            }
            size_t endPos = m_content->find_first_of("]\n", endMark.pos);
            if (endPos != m_content->npos && m_content->at(endPos) == ']')
            {
                int nextPos = int(endPos + 1);
                endMark.column += nextPos - endMark.pos;
                endMark.pos = nextPos;
            }
            LOG4CXX_TRACE(m_log, indent << endMark << "EndSeq");
            break;
        }
        case NodeType::Map:
        {
            LOG4CXX_TRACE(m_log, indent << key.mark << "BeginMap");
            StringType keyIndent = indent + "  ";
            StringType valueIndent = keyIndent + ":  ";
            std::set<StringType> keys;
            for (Node::const_iterator item = root.begin(); item != root.end(); ++item)
            {
                if (item->first.IsScalar() && !keys.insert(item->first.Scalar()).second)
                    throw DuplicateKey(item->first.Mark(), item->first.Scalar());
                Mark keyStart = item->first.Mark();
                Mark keyEnd;
                if (keyStart.pos < rootLocation.pos) // An alias key?
                {
                    keyStart = rootLocation;
                    size_t startPos = m_content->find_first_of("*\n", keyStart.pos);
                    while (m_content->npos != startPos && '\n' == m_content->at(startPos))
                    {
                        ++keyStart.line;
                        keyStart.column = 0;
                        keyStart.pos = int(startPos + 1);
                        startPos = m_content->find_first_of("*\n", keyStart.pos);
                    }
                    if (m_content->npos != startPos)
                    {
                        int nextPos = int(startPos);
                        keyStart.column += nextPos - keyStart.pos;
                        keyStart.pos = nextPos;
                    }
                    keyEnd = AnchorEnd(keyStart);
                    LOG4CXX_TRACE(m_log, keyIndent << "keyAlias " << keyStart << " to " << keyEnd);
                }
                else
                    keyEnd = SetNodeIndex(item->first, keyIndent);
                Mark valueEnd = SetNodeIndex(item->second, valueIndent);
                if (valueEnd.pos < keyEnd.pos) // An alias value?
                {
                    Mark valueStart = AnchorStart(keyEnd);
                    valueEnd = AnchorEnd(valueStart);
                    LOG4CXX_TRACE(m_log, valueIndent << "valueAlias " << valueStart << " to " << valueEnd);
                }
                posData.push_back(std::make_pair(keyStart, valueEnd));
                if (endMark.pos < valueEnd.pos) // Not out of order?
                    endMark = valueEnd;
            }
            size_t endPos = m_content->find_first_of("}\n", endMark.pos);
            if (endPos != m_content->npos && m_content->at(endPos) == '}')
            {
                int nextPos = int(endPos + 1);
                endMark.column += nextPos - endMark.pos;
                endMark.pos = nextPos;
            }
            LOG4CXX_TRACE(m_log, indent << endMark << "EndMap");
            break;
        }
    }
    posData[0].second = endMark;
    m_nodeIndex[key] = posData;
    return endMark;
}

/////////////////////////////////////////////////////////////////////////////////
// DocumentStream implementation

    log4cxx::LoggerPtr
DocumentStream::m_log(log4cxx::Logger::getLogger("YamlDocumentStream"));

    const char*
DocumentStream::Separator = "\n---\n";

// Load this index from \c fileName
    std::vector<Node>
DocumentStream::LoadFile(const PathType& fileName)
{
    LOG4CXX_DEBUG(m_log, "LoadFile: " << fileName);
    if (!exists(fileName))
    {
        Foundation::MissingFileException ex(fileName);
        LOG4CXX_WARN(m_log, ex.what() << " in LoadFile");
        throw ex;
    }
    std::ifstream fin(fileName.c_str());
    std::vector<Node> result;
    while (fin)
    {
        m_documentStore.push_back(Document());
        result.push_back(m_documentStore.back().Load(fin));
    }
    return result;
}

// Write this content to \c fileName
    void
DocumentStream::StoreFile(const PathType& fileName) const
{
    LOG4CXX_DEBUG(m_log, "StoreFile: " << fileName);
    std::ofstream fout(fileName.empty() ? m_filePath.c_str() : fileName.c_str());
    for ( DocumentList::const_iterator item = m_documentStore.begin()
        ; item != m_documentStore.end()
        ; ++item)
    {
        if (item != m_documentStore.begin())
            fout << Separator;
        item->Store(fout);
        fout.flush();
        if (fout.bad())
        {
            Foundation::WriteFileException ex(fileName);
            LOG4CXX_WARN(m_log, ex.what() << " in StoreFile");
            throw ex;
        }
    }
}

/////////////////////////////////////////////////////////////////////////////////
// DocumentTemplate implementation

    log4cxx::LoggerPtr
DocumentTemplate::m_log(log4cxx::Logger::getLogger("YamlDocumentTemplate"));

// The indexed and parsed copy of \c originalText
DocumentTemplate::DocumentTemplate(const StringType& originalText)
{
    if (!originalText.empty())
        m_contentNode = m_doc.Load(originalText);
}

// The indexed and parsed copy of \c fileName
DocumentTemplate::DocumentTemplate(const PathType& fileName)
    : m_contentNode(m_doc.LoadFile(fileName))
{
    if (m_log->isTraceEnabled())
    {
        std::stringstream ss;
        m_doc.Store(ss);
        LOG4CXX_DEBUG(m_log, fileName << '\n' << ss.str());
    }
}

// The indexed and parsed copy of \c textStream
DocumentTemplate::DocumentTemplate(std::istream& textStream)
    : m_contentNode(m_doc.Load(textStream))
{
    if (m_log->isTraceEnabled())
    {
        std::stringstream ss;
        m_doc.Store(ss);
        LOG4CXX_DEBUG(m_log, '\n' << ss.str());
    }
}

// A YAML document with values of \c paramOverrides replacing the original \c sectionName values. PRE: IsValid()
    DocumentTemplate
DocumentTemplate::GetAdaptedTemplate(const Node& paramOverrides, const StringType& sectionName) const
{
    DocumentTemplate result;
    result.m_contentNode = result.m_doc.Load(*GetAdaptedText(paramOverrides, sectionName));
    return result;
}

/// The YAML text with values of \c paramOverrides replacing the original parameter values. PRE: IsValid()
    const DocumentTemplate::StringPtr
DocumentTemplate::GetAdaptedText(const Node& paramOverrides, const StringType& sectionName) const
{
    LOG4CXX_DEBUG(m_log, "GetAdaptedText:" << " paramOverrideCount " << paramOverrides.size());
    StringPtr result;
    Node params = m_contentNode[sectionName];
    if (params.IsSequence() && paramOverrides.IsMap() && 0 < paramOverrides.size())
    {
        Document& tempDoc = const_cast<DocumentTemplate*>(this)->m_doc;
        for (auto item : params)
        {
            StringType paramName = tempDoc.GetAnchorName(item.Mark());
            LOG4CXX_DEBUG(m_log, "GetAdaptedText:"
                << " paramPos " << item.Mark().pos
                << " paramName " << paramName
                << " value " << paramOverrides[paramName]
                );
            if (!paramOverrides[paramName])
                ;
            else if (tempDoc.HasScalarAt(item.Mark()))
            {
                Emitter emitter;
                emitter << Flow << paramOverrides[paramName];
                tempDoc.UpdateScalar(item.Mark(), emitter);
            }
            else if (tempDoc.HasSequenceAt(item.Mark()))
            {
                Emitter emitter;
                emitter << Flow << paramOverrides[paramName];
                tempDoc.UpdateSequence(item.Mark(), emitter);
            }
        }
        std::stringstream ss;
        tempDoc.Store(ss);
        result = std::make_shared<StringType>(ss.str());
        tempDoc.ResetUpdates();
    }
    else
        result = m_doc.GetOriginalText();
    LOG4CXX_DEBUG(m_log, "GetAdaptedText:" << '\n' << *result);
    return result;
}

// Is this a valid adaptable document?
    bool
DocumentTemplate::IsValid() const
{
    return m_contentNode.IsMap();
}

} // namespace YAML
