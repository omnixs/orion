#include <boost/test/unit_test.hpp>
#include <orion/common/xml2json.hpp>
#include <nlohmann/json.hpp>

using json = nlohmann::json;
using namespace orion::common;

BOOST_AUTO_TEST_SUITE(xml2json_tests)

BOOST_AUTO_TEST_CASE(test_parse_xml_value_basic_types)
{
    // Test numeric parsing
    auto result = parse_xml_value("42", "xsd:integer");
    BOOST_CHECK_EQUAL(result.get<int>(), 42);
    
    result = parse_xml_value("3.14", "xsd:decimal");
    BOOST_CHECK_CLOSE(result.get<double>(), 3.14, 0.001);
    
    // Test boolean parsing
    result = parse_xml_value("true", "xsd:boolean");
    BOOST_CHECK_EQUAL(result.get<bool>(), true);
    
    result = parse_xml_value("false", "xsd:boolean");
    BOOST_CHECK_EQUAL(result.get<bool>(), false);
    
    // Test string parsing
    result = parse_xml_value("hello", "xsd:string");
    BOOST_CHECK_EQUAL(result.get<std::string>(), "hello");
}

BOOST_AUTO_TEST_CASE(test_parse_simple_test_xml)
{
    std::string simple_xml = R"(
        <testCases xmlns:tc="http://www.omg.org/spec/DMN/20160719/testcase" xmlns="http://www.omg.org/spec/DMN/20160719/testcase">
            <testCase id="001">
                <inputNode name="Age">
                    <value xsi:type="xsd:decimal" xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance">18</value>
                </inputNode>
                <resultNode name="Risk Category" id="RiskCategory">
                    <expected>
                        <value xsi:type="xsd:string" xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance">MEDIUM</value>
                    </expected>
                </resultNode>
            </testCase>
        </testCases>
    )";
    
    auto cases = parse_test_xml(simple_xml);
    BOOST_REQUIRE_EQUAL(cases.size(), 1);
    
    const auto& test_case = cases[0];
    BOOST_CHECK_EQUAL(test_case.id, "001");
    
    // Check input
    BOOST_REQUIRE(test_case.input.contains("Age"));
    BOOST_CHECK_EQUAL(test_case.input["Age"].get<int>(), 18);
    
    // Check output expectation
    BOOST_REQUIRE_EQUAL(test_case.outputs.size(), 1);
    const auto& output = test_case.outputs[0];
    BOOST_CHECK_EQUAL(output.name, "Risk Category");
    BOOST_CHECK_EQUAL(output.id, "RiskCategory");
    BOOST_CHECK_EQUAL(output.expected, "\"MEDIUM\"");
}

BOOST_AUTO_TEST_CASE(test_parse_component_structure)
{
    std::string component_xml = R"(
        <expected>
            <component name="Monthly Salary">
                <value xsi:type="xsd:decimal" xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance">10000</value>
            </component>
            <component name="Name">
                <value xsi:type="xsd:string" xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance">John Doe</value>
            </component>
        </expected>
    )";
    
    auto result = parse_component_structure(component_xml);
    BOOST_REQUIRE(result.contains("Monthly Salary"));
    BOOST_REQUIRE(result.contains("Name"));
    
    BOOST_CHECK_EQUAL(result["Monthly Salary"].get<int>(), 10000);
    BOOST_CHECK_EQUAL(result["Name"].get<std::string>(), "John Doe");
}

BOOST_AUTO_TEST_CASE(test_parse_list_output_structure)
{
    std::string list_xml = R"(
        <testCases xmlns:tc="http://www.omg.org/spec/DMN/20160719/testcase" xmlns="http://www.omg.org/spec/DMN/20160719/testcase">
            <testCase id="002">
                <inputNode name="Age">
                    <value xsi:type="xsd:decimal" xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance">25</value>
                </inputNode>
                <resultNode name="Applicants" id="Applicants">
                    <expected>
                        <list>
                            <item>
                                <component name="Age">
                                    <value xsi:type="xsd:decimal" xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance">25</value>
                                </component>
                                <component name="Name">
                                    <value xsi:type="xsd:string" xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance">Amy</value>
                                </component>
                            </item>
                            <item>
                                <component name="Age">
                                    <value xsi:type="xsd:decimal" xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance">30</value>
                                </component>
                                <component name="Name">
                                    <value xsi:type="xsd:string" xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance">Bob</value>
                                </component>
                            </item>
                        </list>
                    </expected>
                </resultNode>
            </testCase>
        </testCases>
    )";
    
    auto cases = parse_test_xml(list_xml);
    BOOST_REQUIRE_EQUAL(cases.size(), 1);
    
    const auto& test_case = cases[0];
    BOOST_REQUIRE_EQUAL(test_case.outputs.size(), 1);
    
    const auto& output = test_case.outputs[0];
    BOOST_CHECK_EQUAL(output.name, "Applicants");
    
    // Parse the expected JSON array
    auto expected_array = json::parse(output.expected);
    BOOST_REQUIRE(expected_array.is_array());
    BOOST_REQUIRE_EQUAL(expected_array.size(), 2);
    
    // Check first item
    BOOST_CHECK_EQUAL(expected_array[0]["Age"].get<int>(), 25);
    BOOST_CHECK_EQUAL(expected_array[0]["Name"].get<std::string>(), "Amy");
    
    // Check second item
    BOOST_CHECK_EQUAL(expected_array[1]["Age"].get<int>(), 30);
    BOOST_CHECK_EQUAL(expected_array[1]["Name"].get<std::string>(), "Bob");
}

BOOST_AUTO_TEST_SUITE_END()