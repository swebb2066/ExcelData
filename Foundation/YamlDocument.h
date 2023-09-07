// Extensions to the YAML namespace
#ifndef FOUNDATION_YAML_DOCUMENT_HEADER__
#define FOUNDATION_YAML_DOCUMENT_HEADER__
#include <Foundation/Logger.h>
#include <boost/filesystem.hpp>
#include <boost/shared_ptr.hpp>
#include <yaml-cpp/yaml.h>
#include <vector>
#include <map>

/// Extensions to yaml-cpp
namespace YAML
{
// The name corresponding to \c type
    std::string
to_string(NodeType::value type);

/// Support for round tripping a YAML document
class Document
{
public: // Types
    typedef std::string StringType;
    typedef boost::shared_ptr<StringType> StringPtr;
    typedef boost::filesystem::path PathType;
    typedef std::vector<StringType> StringStore;

    /// A key for updating an update
    class UpdateMark : public Mark
    {
        friend class Document;
    private: // Attributes
        std::vector<size_t> updatePath; //!< Indices into Document::m_updateStore from deepest level to shallowest level

    public: // ...structors
        UpdateMark() : Mark(null_mark()) {}
        UpdateMark(const Mark& base) : Mark(base) {}

    public: // operators
        /// Is this a valid location?
        typedef bool (Document::*unspecified_bool_type)() const;
        operator unspecified_bool_type() const { return is_null() ? 0 : &Document::HasUpdates; }

    private: // ...structors
            template <class FwdIter>
        UpdateMark(const Mark& base, const FwdIter& begin, const FwdIter& end)
            : Mark(base)
            , updatePath(begin, end)
            {}

    private: // Accessors
        bool HasInternalIndex() const { return !updatePath.empty(); }
        size_t InternalIndex() const { return updatePath.back(); }
        UpdateMark InternalUpdateMark() const;
    };

    /// Produces keys for updating an update
    class UpdateNode
    {
    private: // Attributes
        Node yaml; //!< The parsed version of the emitted change
        std::vector<size_t> updatePath; //!< Indices into Document::m_updateStore from deepest level to shallowest level
        StringPtr newText; //!< For delayed initialization of yaml

    public: // ...structors
        /// An uninitialized update
        UpdateNode() : yaml(NodeType::Undefined) {}

        /// A node with locations which refer to the base document (not an update to the base document)
        UpdateNode(const Node& data);

    public: // operators
        /// Is this a valid node?
        typedef bool (Document::*unspecified_bool_type)() const;
        operator unspecified_bool_type() const { return (!yaml.IsDefined() && !newText) ? 0 : &Document::HasUpdates; }

        /// An updatable reference to the value of \c key in the emitted change
        template<typename Key> UpdateNode operator[](const Key& key) const
        { return UpdateNode(this->Data()[key], this->updatePath); }

    public: // Accessors
        /// The parsed version of the emitted change
        const Node& Data() const;

        /// The text of this update
        const StringType& GetNewText() const { return *newText; }

        /// Does this hold un-updated text?
        bool HasUnchangedText() const { return newText && !yaml.IsDefined(); }

        /// A key to the emitted change
        UpdateMark Mark() const;

        /// The number of elements in the emitted change
        size_t size() const;

        /// Put a textual version of this update onto \c os
        void Write(std::ostream& os) const;

    public: // Property modifiers
        /// Use \c emittedText as the change to the base document. PRE: HasUnchangedText()
        void ChangeTextTo(const std::string& emittedText);

    private: // ...structors
        /// A node in an emitted change at \c path in the base document
        UpdateNode(const Node& data, const std::vector<size_t>& path);

        /// A emitted change to the base document
        UpdateNode(const StringPtr& text);

        friend class Document;
    };

protected: // Types
    struct NodePosition
    {
        Mark mark;
        NodeType::value type;
        int additionPos;

        bool operator<(const NodePosition& other) const
        { return mark.pos < other.mark.pos ||
                ( mark.pos == other.mark.pos && int(type) < int(other.type) ) ||
                ( mark.pos == other.mark.pos && type == other.type && additionPos < other.additionPos );
        }

        bool operator==(const NodePosition& other) const
        { return mark.pos == other.mark.pos && type == other.type && additionPos == other.additionPos; }
    };

    typedef std::pair<Mark, Mark> StartEndData;
    typedef std::vector<StartEndData> StartEndDataStore;
    typedef std::map<NodePosition, StartEndDataStore> NodeTextMap;
    enum EditType { Append, Insert, Modify };
    struct UpdateData
    {
        EditType type;
        int indent;
        size_t docIndex;
        Mark resumeAt;
    };
    typedef std::map<NodePosition, UpdateData> StartToUpdateMap;
    typedef std::vector<Document> DocumentList;

protected: // Attributes
    NodeType::value m_rootType; //!< Required for addition to an empty document
    StringPtr m_content; //!< The text at load time
    NodeTextMap m_nodeIndex; //!< From YAML node to a position in m_content
    StartToUpdateMap m_updateIndex; //!< The edits to apply when storing
    DocumentList m_updateStore; //!< Indexed edit text

public: //...structors
    /// An empty document with a \c rootType at the top level
    Document(const NodeType::value& rootType = NodeType::Undefined);
    /// A map document with \c subDoc as value for \c key
    Document(const StringType& key, const Document& subDoc);

public: // Accessors
    /// The anchor name at \c location
    StringType GetAnchorName(const Mark& location) const;

    /// The (unindented) text of the \c type value at \c location. Precondition: !IsEmpty()
    StringType GetItem(const NodeType::value& type, const UpdateMark& location) const;

    /// The number of items in the \c type node at \c location
    size_t GetItemCount(const NodeType::value& type, const UpdateMark& location) const;

    /// The (unindented) text of the items at \c location. Precondition: !IsEmpty()
    StringStore GetItems(const NodeType::value& type, const UpdateMark& location) const;

    /// The end position of the \c type text at \c location
    Mark GetItemEnd(const NodeType::value& type, const UpdateMark& location) const;

    /// The number of items in the map node at \c location
    size_t GetMapItemCount(const UpdateMark& location) const;

    /// The (unindented) text of the items at \c location. Precondition: !IsEmpty()
    StringStore GetMapItems(const UpdateMark& location) const;

    /// The unmodified original text of this document. Precondition: !IsEmpty()
    const StringPtr& GetOriginalText() const { return m_content; }

    /// The number of items in the sequence node at \c location
    size_t GetSequenceItemCount(const UpdateMark& location) const;

    /// The (unindented) text of the items at \c location
    StringStore GetSequenceItems(const UpdateMark& location) const;

    /// The tag name at \c location
    StringType GetTagName(const Mark& location) const;

    /// The tag name at \c valueNode
    StringType GetTagName(const Node& valueNode) const;

    /// Get update \c docNumber (in 1..GetUpdateCount()). Precondition: HasUpdate(docNumber)
    Document& GetUpdate(size_t docNumber) { return m_updateStore[docNumber - 1]; }

    /// Is there a map at \c location?
    bool HasMapAt(const UpdateMark& location) const { return HasTypeAt(NodeType::Map, location); }

    /// Is there a null at \c location?
    bool HasNullAt(const UpdateMark& location) const { return HasTypeAt(NodeType::Null, location); }

    /// Is there a scalar at \c location?
    bool HasScalarAt(const UpdateMark& location) const { return HasTypeAt(NodeType::Scalar, location); }

    /// Is there a sequence at \c location?
    bool HasSequenceAt(const UpdateMark& location) const { return HasTypeAt(NodeType::Sequence, location); }

    /// Is there a \c type item at \c location?
    bool HasTypeAt(const NodeType::value& type, const UpdateMark& location) const;

    /// Is \c docNumber a valid update?
    bool HasUpdate(size_t docNumber) const { return 0 < docNumber && docNumber <= m_updateStore.size(); }

    /// Has this been updated?
    bool HasUpdates() const { return !m_updateIndex.empty(); }

    /// Is this document empty?
    bool IsEmpty() const { return !m_content || m_content->empty(); }

    /// Write content to \c os. Precondition: !IsEmpty()
    void Store(std::ostream& os) const;

    // Write this content to \c fileName. Precondition: !IsEmpty()
    void StoreFile(const PathType& fileName) const;

public: // Methods
    /// Add \c newVal at the end of the map node at \c location.
    UpdateNode AppendTo(const NodeType::value& type, const UpdateMark& location, const StringType& newVal);

    /// Add \c newVal at the end of the map node at \c location.
    UpdateNode AppendToMap(const Emitter& newVal, const UpdateMark& location);

    /// Add \c newVal at the end of the sequence node at \c location.
    UpdateNode AppendToSequence(const Emitter& newVal, const UpdateMark& location);

    /// Add \c newVal before \c atItem (in [1..GetItemCount(type, location)]) in the \c type node at \c location.
    UpdateNode InsertIn(const StringType& newVal, const NodeType::value& type, const UpdateMark& location, size_t atItem = 0);

    /// Add \c newVal before \c atItem (in [1..GetMapItemCount(location)]) in the map node at \c location.
    UpdateNode InsertInMap(const Emitter& newVal, const UpdateMark& location, size_t atItem = 0);

    /// Add \c newVal before \c atItem (in [1..GetSequenceItemCount(location)]) in the sequence node at \c location.
    UpdateNode InsertInSequence(const Emitter& newVal, const UpdateMark& location, size_t atItem = 0);

    /// Merge the map elements in \c newVal into the map node at \c location.
    UpdateNode MergeIntoMap(const Emitter& newVal, const UpdateMark& location);

    /// Set this index from \c content
    Node Load(const StringType& content);

    /// Load this index from \c input
    Node Load(std::istream& input);

    /// Set this index from \c fileName
    Node LoadFile(const PathType& fileName);

    /// Set \c paramName to \c paramValue in the map at \c sectionLocation recording the location for subsequent updates into \c itemLocation
    template <typename T>
    void PutStreamableParameter
        ( const UpdateMark& sectionLocation
        , const StringType& paramName
        , T                 paramValue
        , UpdateMark&       itemLocation
        );

    /// Replace the \c type value at \c location with the null value
    UpdateNode Remove(const NodeType::value& type, const UpdateMark& location);

    /// Replace the map at \c location with the null value
    UpdateNode RemoveMap(const UpdateMark& location);

    /// Replace the scalar value at \c location with the null value
    UpdateNode RemoveScalar(const UpdateMark& location);

    /// Replace the sequence value at \c location with the null value
    UpdateNode RemoveSequence(const UpdateMark& location);

    /// Remove the change \c node
    bool RemoveUpdate(const UpdateNode& node);

    /// Make \c newVal the value at \c location
    UpdateNode Update(const NodeType::value& type, const UpdateMark& location, const StringType& newVal);

    /// Change the map at \c location to \c newVal
    UpdateNode UpdateMap(const UpdateMark& location, const Emitter& newVal);

    /// Change the null at \c location to \c newVal
    UpdateNode UpdateNull(const UpdateMark& location, const Emitter& newVal);

    /// Change the scalar at \c location to \c newVal
    UpdateNode UpdateScalar(const UpdateMark& location, const Emitter& newVal);

    /// Change the sequence at \c location to \c newVal
    UpdateNode UpdateSequence(const UpdateMark& location, const Emitter& newVal);

    /// Change this into an empty document
    void Reset();

    /// Change this into the original document
    void ResetUpdates();

protected: // Support methods
    /// The location of the first character after any anchor (if any) starting at \c mark
    Mark AnchorEnd(const Mark& mark) const;

    /// The location of the first character of an anchor starting at \c mark
    Mark AnchorStart(const Mark& mark) const;

    /// The location of the first character after null starting at \c mark
    Mark NullEnd(const Mark& mark) const;

    /// The Yaml items starting at \c location
    const StartEndDataStore& GetItemsAt(const NodePosition& location) const;

    /// The text at \c startEnd with any indent removed. Precondition: !IsEmpty()
    StringType GetUnindentedItemAt(const StartEndData& startEnd) const;

    /// Is \c location in \c m_nodeIndex?
    bool HasItemsAt(const NodePosition& location) const;

    /// The parsed representation of \c m_content. Precondition: !IsEmpty()
    Node ParseContent();

    /// Remove the \c docIndex change
    bool RemoveUpdate(size_t docIndex);

    /// Set \c m_nodeIndex from \c root
    Mark SetNodeIndex(const Node& root, const StringType& indent = StringType());

    /// Add \c newVal as an \c editType change at \c location
    UpdateNode Update(const NodePosition& location, const StringType& newVal, EditType editType, size_t atIndex = 0);

    /// The name corresponding to \c type
    static StringType ToName(EditType type);

private: // Class attributes
    static log4cxx::LoggerPtr m_log;

public: // Constants
    static const size_t MAX_LINE_SIZE = 2000;
};

/// Put a textual version of \c node onto \c os
    inline std::ostream&
operator<<(std::ostream& os, const Document::UpdateNode& node)
{
    node.Write(os);
    return os;
}

/// Set \c paramName to \c paramValue in the map at \c sectionLocation
/// recording the location for subsequent updates into \c itemLocation
template <typename T>
    void
Document::PutStreamableParameter
    ( const UpdateMark& sectionLocation
    , const StringType& paramName
    , T                 paramValue
    , UpdateMark&       itemLocation
    )
{
    LOG4CXX_DEBUG(m_log, "PutStreamableParameter: " << paramName << " value " << paramValue);
    if (itemLocation)
    {
        Emitter emitter;
        emitter << paramValue;
        LOG4CXX_DEBUG(m_log, "PutStreamableParameter: update " << emitter.c_str());
        if (HasMapAt(itemLocation))
            itemLocation = UpdateMap(itemLocation, emitter).Mark();
        else if (HasSequenceAt(itemLocation))
            itemLocation = UpdateSequence(itemLocation, emitter).Mark();
        else if (HasNullAt(itemLocation))
            itemLocation = UpdateNull(itemLocation, emitter).Mark();
        else if (HasScalarAt(itemLocation))
            itemLocation = UpdateScalar(itemLocation, emitter).Mark();
        else
            itemLocation = UpdateMap(itemLocation, emitter).Mark();
    }
    else if (sectionLocation)
    {
        Emitter emitter;
        emitter << BeginMap;
        emitter << Key << paramName << Value << paramValue;
        emitter << EndMap;
        LOG4CXX_DEBUG(m_log, "PutStreamableParameter: add " << emitter.c_str());
        itemLocation = AppendToMap(emitter, sectionLocation)[paramName].Mark();
    }
}

/// Support for round tripping a YAML file
class DocumentStream
{
public: // Types
    typedef std::string StringType;
    typedef boost::filesystem::path PathType;

public: // Constants
    static const char* Separator;

protected: // Types
    typedef std::list<Document> DocumentList;

protected: // Attributes
    PathType m_filePath; //!< The name of the file used in the last LoadFile operation
    DocumentList m_documentStore; //!< The sequence of documents

public: // Accessors
    /// The name of the file used in the last LoadFile operation
    const PathType& GetFilePath() const { return m_filePath; }

    /// Write this content to \c fileName
    void StoreFile(const PathType& fileName = PathType()) const;

public: // Methods
    /// Load \c m_documents from \c fileName
    std::vector<Node> LoadFile(const PathType& fileName);

private: // Class attributes
    static log4cxx::LoggerPtr m_log;
};

/// Holds indexed object definition text and the corresponding YAML
class DocumentTemplate
{
public: // Types
    typedef std::string StringType;
    typedef boost::shared_ptr<StringType> StringPtr;
    typedef boost::filesystem::path PathType;

protected: // Attributes
    Document m_doc; //!< The indexed original text
    Node m_contentNode; //!< The parsed version of the original text

public: // ...structors
    /// The indexed and parsed copy of \c originalText
    DocumentTemplate(const StringType& originalText = StringType());

    /// The indexed and parsed copy of \c fileName
    DocumentTemplate(const PathType& fileName);

    /// The indexed and parsed copy of \c textStream
    DocumentTemplate(std::istream& textStream);

public: // Property accessors
    /// The original parsed content
    Node GetOriginalData() const { return m_contentNode; }

    /// The original indexed text
    const Document& GetOriginalDocument() const { return m_doc; }

    /// The original object definition text
    const StringPtr& GetOriginalText() const { return m_doc.GetOriginalText(); }

public: // Accessors
    /// A YAML document with values of \c paramOverrides replacing the original \c sectionName values. PRE: IsValid()
    DocumentTemplate GetAdaptedTemplate(const Node& paramOverrides, const StringType& sectionName = "Parameters") const;

    /// The YAML text with values of \c paramOverrides replacing the original \c sectionName values. PRE: IsValid()
    const StringPtr GetAdaptedText(const Node& paramOverrides, const StringType& sectionName = "Parameters") const;

    /// Is this a valid adaptable document?
    bool IsValid() const;

private: // Class attributes
    static log4cxx::LoggerPtr m_log;
};

} // namespace YAML

#endif // FOUNDATION_YAML_DOCUMENT_HEADER__
