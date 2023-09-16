#include <Foundation/Environment.h>
#include <Foundation/YamlDocument.h>
#include <Foundation/LogMessages.h>
#include <Foundation/StdLogger.h>
#include <Foundation/Memory.h>
#include <boost/program_options.hpp>
#include <iostream>
#include <fstream>
#include <filesystem>
#include "CellRowIterator.h"

static const char* usageMsg =
"Search .xls or .xlsx files\n"
"Allowed options"
;

static const char* inputPathParam =
"The directory to search for input data.\n";

static const char* inputPatternParam =
"The name of a file containing a (yaml format) list of parameter values.\n"
"Each list element specifies a variable name and list of value.\n"
"Output is generated for each combination in the cross product of variable values.\n"
"Example 1:\n"
"----------\n"
"\n"
"- Name: NamePrefix\n"
"  Value: [19, 20]\n"
"- Name: Location\n"
"  Value: [Family Trust, Super Fund]\n"
"---\n"
"Example 2:\n"
"----------\n"
"- Name: NamePrefix\n"
"  Value: [ 2016, 2017, 2018, 2019, 2020, 2021, 2022, 2023 ]\n"
"---\n"
"# The list of parameter values may alternatively be provided\n"
"# in the 'ParameterCombinations' section of the map file.\n"
;

static const char* mapFileParam =
"The name (default excel2csv.yaml) of a (yaml format) file\n"
"mapping the input yaml to the output comma separated values.\n"
"Each element in the input list has a variable name and path to the value.\n"
"Each element in the output list specifies a name and a value (optionally as a reverse polish calculation).\n"
"Example 1: Extract all transaction sheets\n"
"----------\n"
"Parameters:\n"
"  - &NamePrefix xxxxx\n"
"  - &Location xxxxx\n"
"Input:\n"
"  Path: [*Location, \"*/\", *NamePrefix, \" Personal Expenses.xls\"]\n"
"  Sheet: Transactions\n"
"---\n"
"Example 2: Extract totals\n"
"----------\n"
"Parameters:\n"
"  - &NamePrefix xxxxxx\n"
"Input:\n"
"  Path: [*NamePrefix, \"*Personal Expenses.xls\"]\n"
"  Sheet: Transactions\n"
"Accumulate:\n"
"  - Name: TotalPlannedArea\n"
"    Value: !area [PartTrackArea]\n"
"  - Name: TotalBadAngleArea\n"
"    Value: !area [ExtremeAngleToSurfacePortion, PartTrackArea, x]\n"
"  - Name: TotalTargetedArea\n"
"    Value: !area [SectionTrackArea]\n"
"  - Name: Duration\n"
"    Value: !duration [TotalDuration]\n"
"Output:\n"
"  - Name: Task\n"
"    Value: *NamePrefix\n"
"    IsKey: true\n"
"  - Name: Total Targeted Area (m^2)\n"
"    Value: !area [TotalTargetedArea]\n"
"  - Name: Total Planned Area (m^2)\n"
"    Value: !area [TotalPlannedArea]\n"
"  - Name: Planned %\n"
"    Value: !area [TotalTargetedArea, TotalPlannedArea, /, 100, x]\n"
"  - Name: Bad Angle Area (m^2)\n"
"    Value: !area [TotalBadAngleArea]\n"
"  - Name: Effective %\n"
"    Value: !area [TotalTargetedArea, TotalBadAngleArea, TotalPlannedArea, -, /, 100, x]\n"
"  - Name: Total Duration (hrs)\n"
"    Value: !duration [3600, 2, Duration, /, /]\n"
"---\n"
"Example 3: Extract totals from selected nodes\n"
"----------\n"
"ParameterCombinations:\n"
"- Name: Location\n"
"  Value: [ Front, Back ]\n"
"- Name: Dataset\n"
"  Value: [ Wagon_1, Wagon_2, Wagon_3, Wagon_4, Wagon_5\n"
"         , Wagon_6, Wagon_7, Wagon_8, Wagon_9, Wagon_10]\n"
"Parameters:\n"
"  - &Location xxxxxx\n"
"  - &Dataset dddddd\n"
"InputPath: [*Location, /, *Location, \" \", *Dataset, \" */*Personal Expenses.xls\"]\n"
"Select:\n"
"  SectionName: [., Section, \".*Torsion Box\"]\n"
"Input:\n"
"  ExtremeAngleToSurfacePortion: [Parts,.,ExtremeAngleToSurfacePortion]\n"
"  PartTrackArea: [Parts,.,TrackArea]\n"
"  SectionDuration: [Duration]\n"
"  SectionTrackArea: [TrackArea]\n"
"Accumulate:\n"
"  - Name: TotalPlannedArea\n"
"    Value: !area [PartTrackArea]\n"
"  - Name: TotalBadAngleArea\n"
"    Value: !area [ExtremeAngleToSurfacePortion, PartTrackArea, x]\n"
"  - Name: TotalTargetedArea\n"
"    Value: !area [SectionTrackArea]\n"
"  - Name: TotalDuration\n"
"    Value: !duration [SectionDuration]\n"
"Output:\n"
"  - Name: Torsion Box\n"
"    Value: [ *Location, \" \", *Dataset ]\n"
"    IsKey: true\n"
"  - Name: Total Targeted Area (m^2)\n"
"    Value: !area [TotalTargetedArea]\n"
"  - Name: Total Planned Area (m^2)\n"
"    Value: !area [TotalPlannedArea]\n"
"  - Name: Planned %\n"
"    Value: !area [TotalTargetedArea, TotalPlannedArea, /, 100, x]\n"
"  - Name: Bad Angle Area (m^2)\n"
"    Value: !area [TotalBadAngleArea]\n"
"  - Name: Effective %\n"
"    Value: !area [TotalTargetedArea, TotalBadAngleArea, TotalPlannedArea, -, /, 100, x]\n"
"  - Name: Total Duration (hrs)\n"
"    Value: !duration [3600, TotalDuration, /]\n"
;

/// Provides a YAML map for each unique combination
class MappingIterator : public Foundation::Iterator<YAML::Node>
{
    using NodePair = std::pair<YAML::Node, YAML::Node>;
protected: // Attributes
    std::vector<NodePair> m_params; //!< The source of component values
    std::vector<size_t> m_itemNumber; //!< The progress through component values

public: // ...structors
    MappingIterator(const YAML::Node data)
    {
        if (data.IsSequence())
        {
            for (auto item : data)
                m_params.push_back(NodePair(item["Name"], item["Value"]));
        }
        else if (data.IsMap())
        {
            for (auto item : data)
                m_params.push_back(NodePair(item.first, item.second));
        }
    }

public: // Accessors
    /// Is this iterator beyond the end or before the start?
    bool Off() const
    {
        return m_itemNumber.empty();
    }

public: // Methods
    /// Move to the first item
    void Start()
    {
        LOG4CXX_TRACE(m_log, "Start:");
        if (m_params.empty())
            return;
        m_itemNumber.clear();
        m_itemNumber.push_back(1);
        if (SetItem())
            LOG4CXX_DEBUG(m_log, "At: " << m_itemNumber);
        else
            m_itemNumber.clear();
    }

    /// Move to the next item. Precondition: !Off()
    void Forth()
    {
        ++m_itemNumber.back();
        if (SetItem())
            LOG4CXX_DEBUG(m_log, "At: " << m_itemNumber);
        else
            m_itemNumber.clear();
    }

protected: // Support methods
    /// Set the m_item and m_itemNumber to the next set of map values. Precondition: !m_itemNumber.empty()
    bool SetItem()
    {
        LOG4CXX_TRACE(m_log, "SetItem: " << m_itemNumber);
        // Ascend
        while (m_params[m_itemNumber.size() - 1].second.size() < m_itemNumber.back())
        {
            m_itemNumber.pop_back();
            if (m_itemNumber.empty())
                return false;
            ++m_itemNumber.back();
        }

        // Descend
        while (m_itemNumber.back() <= m_params[m_itemNumber.size() - 1].second.size())
        {
            auto& keyValue = m_params[m_itemNumber.size() - 1];
            if (!keyValue.second.IsSequence())
                return false;
            m_item[keyValue.first] = keyValue.second[m_itemNumber.back() - 1];
            LOG4CXX_DEBUG(m_log, "SetItem: " << keyValue.first.Scalar()
                << '=' << keyValue.second[m_itemNumber.back() - 1].Scalar()
                );
            if (m_params.size() == m_itemNumber.size())
                return true;
            m_itemNumber.push_back(1);
        }
        return false;
    }

private: // Class data
    static log4cxx::LoggerPtr m_log;
};

    log4cxx::LoggerPtr
MappingIterator::m_log(log4cxx::Logger::getLogger("MappingIterator"));

// Put \c output field values onto \c os
    void
OutputLine(std::ostream& os, const Excel::CellRow& output)
{
    auto fieldCount = 0;
    for (auto field : output)
    {
        if (0 < fieldCount)
            os << ',';
        os << field;
        ++fieldCount;
    }
    os << '\n';
}

// Put the value in \c node onto \c os after conversion using the type implied by to \c tagName
    void
OutputValue(std::ostream& os, const std::string& tagName, const YAML::Node& node)
{
    static log4cxx::LoggerPtr log_s(log4cxx::Logger::getLogger("OutputValue"));
    os << '"';
    if (node.IsScalar())
        os << node.Scalar();
    else if (node.IsSequence())
    {
        for (auto item : node)
            if (item.IsScalar())
                os << item.Scalar();
    }
    os << '"';
}

// Put the values onto \c os from documents in \c inputPath using \c mapping
    void
ProcessDocuments(std::ostream& os, const fs::path& inputPath, const YAML::DocumentTemplate& mapping)
{
    static auto log_s(log4cxx::Logger::getLogger("ProcessDocuments"));
    auto mappingData = mapping.GetOriginalData();
    auto mappingDoc = mapping.GetOriginalDocument();
    Excel::CellRowIterator input(inputPath, mappingData["Input"]);
    auto output = mappingData["Output"];
    std::vector<std::string> lastKey;
    for (input.Start(); !input.Off(); input.Forth())
    {
        LOG4CXX_DEBUG(log_s, "lineItemSize " << input.Item().size());
        auto keyIndex = 0;
        bool keyChanged = false;
        for (auto field : output)
        {
            auto valueNode = field["Value"];
            auto keyNode = field["IsKey"];
            if (keyNode.IsScalar() && keyNode.as<bool>())
            {
                std::stringstream ss;
                OutputValue(ss, mappingDoc.GetTagName(valueNode), valueNode);
                auto value = ss.str();
                if (lastKey.size() <= keyIndex)
                    lastKey.push_back(value);
                else
                {
                    keyChanged = keyChanged || (value != lastKey[keyIndex]);
                    lastKey[keyIndex] = value;
                }
                ++keyIndex;
            }
        }
        if (0 == keyIndex || keyChanged)
        {
            OutputLine(os, input.Item());
        }
    }
}

namespace po = boost::program_options;

/// A command line interface for extracting comma separated values from Excel files
int main(int argc, char* argv[])
{
    // Declare the supported options.

namespace po = boost::program_options;
    po::options_description desc(usageMsg, 160);
    desc.add_options()
        ("help,h", "display optional parameter descriptions")
        ("map-file,m", po::value<std::string>(), mapFileParam)
        ("input-root,r", po::value<std::string>(), inputPathParam)
        ("map-params,p", po::value<std::string>(), inputPatternParam);
    po::positional_options_description p;
    p.add("map-file", -1);

    // Load options.
    po::variables_map vm;
    try
    {
        po::store
            ( po::command_line_parser(argc, argv)
                .options(desc)
                .positional(p).run()
            , vm
            );
    }
    catch (std::exception& ex)
    {
        std::cerr << desc << "\n";
        return 1;
    }

    // Run all 'notifier' functions in 'vm'
    po::notify(vm);

    if (vm.count("help"))
    {
        std::cerr << desc << "\n";
        return 1;
    }

    fs::path inputPath;
    if (0 < vm.count("input-root"))
    {
        inputPath = vm["input-root"].as<std::string>();
        std::error_code ec;
        fs::current_path(inputPath, ec);
    }

    auto log_s = Foundation::GetLogger("excel2csv");
    // Separate this instance of log messages
    Foundation::UserAndVersionLogMessage(log_s);
    LOG4CXX_DEBUG(log_s, "inputPath " << inputPath);

    fs::path mapfile;
    if (0 == vm.count("map-file"))
        mapfile = Foundation::Environment::GetConfigFile(".yaml");
    else
        mapfile = vm["map-file"].as<std::string>();

    if (!exists(mapfile))
    {
        LOG4CXX_ERROR(log_s, mapfile << ": not found");
        std::cerr << mapfile << ": not found\n";
        return 1;
    }
    LOG4CXX_INFO(log_s, "Mapping configured from " << mapfile);
    HeapChangeLogger h(log_s);
    fs::path paramfile;
    if (0 < vm.count("map-params"))
    {
        paramfile = vm["map-params"].as<std::string>();
        if (!inputPath.empty() && exists(inputPath) && exists(inputPath / paramfile))
            paramfile = inputPath / paramfile;
        if (!exists(paramfile))
        {
            LOG4CXX_ERROR(log_s, paramfile << ": not found");
            std::cerr << paramfile << ": not found\n";
            return 1;
        }
        LOG4CXX_INFO(log_s, "Mapping parameters from " << paramfile);
    }
    try
    {
        std::ifstream in(mapfile.c_str());
        YAML::DocumentTemplate mapData(in);
        auto data = mapData.GetOriginalData();
        auto fieldCount = 0;
        for (auto field : data["Output"])
        {
            if (0 < fieldCount)
                std::cout << ',';
            auto nameNode = field["Name"];
            if (nameNode.IsScalar())
                std::cout << '"' << nameNode.Scalar() << '"';
            ++fieldCount;
        }
        std::cout << '\n';
        YAML::Node paramCombinations;
        if (!paramfile.empty())
            paramCombinations = YAML::LoadFile(paramfile.string());
        else
            paramCombinations = data["ParameterCombinations"];
        if (0 < paramCombinations.size())
        {
            MappingIterator paramInput(paramCombinations);
            for (paramInput.Start(); !paramInput.Off(); paramInput.Forth())
            {
                LOG4CXX_DEBUG(log_s, "paramSize " << paramInput.Item().size());
                auto instance = mapData.GetAdaptedTemplate(paramInput.Item(), "Parameters");
                ProcessDocuments(std::cout, inputPath, instance);
            }
        }
        else
            ProcessDocuments(std::cout, inputPath, mapData);
    }
    catch (std::exception& ex)
    {
        LOG4CXX_ERROR(log_s, ex.what());
        std::cerr << ex.what() << "\n";
        return 1;
    }
    h("exit");
    Foundation::CleanExitLogMessage(log_s);
    return 0;
}
