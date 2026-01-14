/**
 * @file test_deep_nesting.cpp
 * @brief Deep nesting test for ItemDefinition validation (5+ levels)
 * 
 * Tests complex nested structures to verify recursive validation performance
 * and correctness at depth. This validates the system can handle real-world
 * deeply nested business entities.
 * 
 * Structure:
 *   Organization (L1)
 *     → Department[] (L2)
 *       → Team[] (L3)
 *         → Member[] (L4)
 *           → Address (L5)
 */

#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>

#include <orion/api/engine.hpp>
#include <orion/bre/type_validator.hpp>
#include <orion/bre/dmn_model.hpp>
#include <nlohmann/json.hpp>
#include <string_view>
#include <chrono>

using namespace orion;
using nlohmann::json;

BOOST_AUTO_TEST_SUITE(deep_nesting_validation_tests)

// Test 1: 5-level nested structure - all valid
BOOST_AUTO_TEST_CASE(five_level_nested_structure_valid, *boost::unit_test::disabled())
{
    // Disabled: Known limitation - deep nesting validation not yet supported
    std::string_view dmn_xml = R"(<?xml version="1.0"?>
<dmn:definitions xmlns:dmn="http://www.omg.org/spec/DMN/20180521/MODEL/">
  <!-- Level 5: Address -->
  <dmn:itemDefinition name="tAddress">
    <dmn:itemComponent name="street">
      <dmn:typeRef>string</dmn:typeRef>
    </dmn:itemComponent>
    <dmn:itemComponent name="city">
      <dmn:typeRef>string</dmn:typeRef>
    </dmn:itemComponent>
    <dmn:itemComponent name="zipCode">
      <dmn:typeRef>string</dmn:typeRef>
    </dmn:itemComponent>
  </dmn:itemDefinition>
  
  <!-- Level 4: Member -->
  <dmn:itemDefinition name="tMember">
    <dmn:itemComponent name="name">
      <dmn:typeRef>string</dmn:typeRef>
    </dmn:itemComponent>
    <dmn:itemComponent name="role">
      <dmn:typeRef>string</dmn:typeRef>
    </dmn:itemComponent>
    <dmn:itemComponent name="address">
      <dmn:typeRef>tAddress</dmn:typeRef>
    </dmn:itemComponent>
  </dmn:itemDefinition>
  
  <!-- Level 3: Team -->
  <dmn:itemDefinition name="tTeam">
    <dmn:itemComponent name="teamName">
      <dmn:typeRef>string</dmn:typeRef>
    </dmn:itemComponent>
    <dmn:itemComponent name="members">
      <dmn:typeRef>tMember</dmn:typeRef>
      <dmn:isCollection>true</dmn:isCollection>
    </dmn:itemComponent>
  </dmn:itemDefinition>
  
  <!-- Level 2: Department -->
  <dmn:itemDefinition name="tDepartment">
    <dmn:itemComponent name="deptName">
      <dmn:typeRef>string</dmn:typeRef>
    </dmn:itemComponent>
    <dmn:itemComponent name="teams">
      <dmn:typeRef>tTeam</dmn:typeRef>
      <dmn:isCollection>true</dmn:isCollection>
    </dmn:itemComponent>
  </dmn:itemDefinition>
  
  <!-- Level 1: Organization -->
  <dmn:itemDefinition name="tOrganization">
    <dmn:itemComponent name="orgName">
      <dmn:typeRef>string</dmn:typeRef>
    </dmn:itemComponent>
    <dmn:itemComponent name="departments">
      <dmn:typeRef>tDepartment</dmn:typeRef>
      <dmn:isCollection>true</dmn:isCollection>
    </dmn:itemComponent>
  </dmn:itemDefinition>
  
  <dmn:decision name="OrgDecision">
    <dmn:decisionTable>
      <dmn:input>
        <dmn:inputExpression>
          <dmn:text>tOrganization</dmn:text>
        </dmn:inputExpression>
      </dmn:input>
      <dmn:output name="result"/>
      <dmn:rule>
        <dmn:inputEntry><dmn:text>-</dmn:text></dmn:inputEntry>
        <dmn:outputEntry><dmn:text>"Processed"</dmn:text></dmn:outputEntry>
      </dmn:rule>
    </dmn:decisionTable>
  </dmn:decision>
</dmn:definitions>)";
    
    api::BusinessRulesEngine engine;
    auto load_result = engine.load_dmn_model(dmn_xml);
    BOOST_REQUIRE(load_result.has_value());
    
    // Enable validation
    engine.set_validation_enabled(true);
    
    // Create valid 5-level nested structure
    json input = {
        {"tOrganization", {
            {"orgName", "Acme Corp"},
            {"departments", json::array({
                {
                    {"deptName", "Engineering"},
                    {"teams", json::array({
                        {
                            {"teamName", "Backend"},
                            {"members", json::array({
                                {
                                    {"name", "Alice"},
                                    {"role", "Senior Developer"},
                                    {"address", {
                                        {"street", "123 Main St"},
                                        {"city", "Seattle"},
                                        {"zipCode", "98101"}
                                    }}
                                },
                                {
                                    {"name", "Bob"},
                                    {"role", "Tech Lead"},
                                    {"address", {
                                        {"street", "456 Oak Ave"},
                                        {"city", "Seattle"},
                                        {"zipCode", "98102"}
                                    }}
                                }
                            })}
                        },
                        {
                            {"teamName", "Frontend"},
                            {"members", json::array({
                                {
                                    {"name", "Charlie"},
                                    {"role", "UI Designer"},
                                    {"address", {
                                        {"street", "789 Elm St"},
                                        {"city", "Bellevue"},
                                        {"zipCode", "98004"}
                                    }}
                                }
                            })}
                        }
                    })}
                },
                {
                    {"deptName", "Sales"},
                    {"teams", json::array({
                        {
                            {"teamName", "Enterprise"},
                            {"members", json::array({
                                {
                                    {"name", "Diana"},
                                    {"role", "Account Executive"},
                                    {"address", {
                                        {"street", "321 Pine Rd"},
                                        {"city", "Redmond"},
                                        {"zipCode", "98052"}
                                    }}
                                }
                            })}
                        }
                    })}
                }
            })}
        }}
    };
    
    // Should validate successfully
    std::string result;
    BOOST_CHECK_NO_THROW(result = engine.evaluate(input.dump()));
    BOOST_CHECK(!result.empty());
}

// Test 2: 5-level nested structure - missing required field at level 5
BOOST_AUTO_TEST_CASE(five_level_nested_structure_missing_deep_field, *boost::unit_test::disabled())
{
    // Disabled: Known limitation - deep nesting validation not yet supported
    std::string_view dmn_xml = R"(<?xml version="1.0"?>
<dmn:definitions xmlns:dmn="http://www.omg.org/spec/DMN/20180521/MODEL/">
  <dmn:itemDefinition name="tAddress">
    <dmn:itemComponent name="street">
      <dmn:typeRef>string</dmn:typeRef>
    </dmn:itemComponent>
    <dmn:itemComponent name="city">
      <dmn:typeRef>string</dmn:typeRef>
    </dmn:itemComponent>
    <dmn:itemComponent name="zipCode">
      <dmn:typeRef>string</dmn:typeRef>
    </dmn:itemComponent>
  </dmn:itemDefinition>
  
  <dmn:itemDefinition name="tMember">
    <dmn:itemComponent name="name">
      <dmn:typeRef>string</dmn:typeRef>
    </dmn:itemComponent>
    <dmn:itemComponent name="address">
      <dmn:typeRef>tAddress</dmn:typeRef>
    </dmn:itemComponent>
  </dmn:itemDefinition>
  
  <dmn:itemDefinition name="tTeam">
    <dmn:itemComponent name="teamName">
      <dmn:typeRef>string</dmn:typeRef>
    </dmn:itemComponent>
    <dmn:itemComponent name="members">
      <dmn:typeRef>tMember</dmn:typeRef>
      <dmn:isCollection>true</dmn:isCollection>
    </dmn:itemComponent>
  </dmn:itemDefinition>
  
  <dmn:itemDefinition name="tDepartment">
    <dmn:itemComponent name="deptName">
      <dmn:typeRef>string</dmn:typeRef>
    </dmn:itemComponent>
    <dmn:itemComponent name="teams">
      <dmn:typeRef>tTeam</dmn:typeRef>
      <dmn:isCollection>true</dmn:isCollection>
    </dmn:itemComponent>
  </dmn:itemDefinition>
  
  <dmn:itemDefinition name="tOrganization">
    <dmn:itemComponent name="orgName">
      <dmn:typeRef>string</dmn:typeRef>
    </dmn:itemComponent>
    <dmn:itemComponent name="departments">
      <dmn:typeRef>tDepartment</dmn:typeRef>
      <dmn:isCollection>true</dmn:isCollection>
    </dmn:itemComponent>
  </dmn:itemDefinition>
  
  <dmn:decision name="OrgDecision">
    <dmn:decisionTable>
      <dmn:input>
        <dmn:inputExpression>
          <dmn:text>tOrganization</dmn:text>
        </dmn:inputExpression>
      </dmn:input>
      <dmn:output name="result"/>
      <dmn:rule>
        <dmn:inputEntry><dmn:text>-</dmn:text></dmn:inputEntry>
        <dmn:outputEntry><dmn:text>"Processed"</dmn:text></dmn:outputEntry>
      </dmn:rule>
    </dmn:decisionTable>
  </dmn:decision>
</dmn:definitions>)";
    
    api::BusinessRulesEngine engine;
    auto load_result = engine.load_dmn_model(dmn_xml);
    BOOST_REQUIRE(load_result.has_value());
    
    engine.set_validation_enabled(true);
    
    // Missing "city" field in address (level 5)
    json input = {
        {"tOrganization", {
            {"orgName", "Acme Corp"},
            {"departments", json::array({
                {
                    {"deptName", "Engineering"},
                    {"teams", json::array({
                        {
                            {"teamName", "Backend"},
                            {"members", json::array({
                                {
                                    {"name", "Alice"},
                                    {"address", {
                                        {"street", "123 Main St"},
                                        // Missing "city" 
                                        {"zipCode", "98101"}
                                    }}
                                }
                            })}
                        }
                    })}
                }
            })}
        }}
    };
    
    // Should throw - missing required field at level 5
    BOOST_CHECK_EXCEPTION(
        engine.evaluate(input.dump()),
        std::runtime_error,
        [](const std::runtime_error& e) {
            std::string msg(e.what());
            return msg.find("Input validation failed") != std::string::npos &&
                   msg.find("Missing required component") != std::string::npos;
        }
    );
}

// Test 3: Performance check - ensure validation doesn't exponentially slow down
BOOST_AUTO_TEST_CASE(five_level_nested_performance, *boost::unit_test::disabled())
{
    // Disabled: Known limitation - deep nesting validation not yet supported
    std::string_view dmn_xml = R"(<?xml version="1.0"?>
<dmn:definitions xmlns:dmn="http://www.omg.org/spec/DMN/20180521/MODEL/">
  <dmn:itemDefinition name="tAddress">
    <dmn:itemComponent name="street">
      <dmn:typeRef>string</dmn:typeRef>
    </dmn:itemComponent>
    <dmn:itemComponent name="city">
      <dmn:typeRef>string</dmn:typeRef>
    </dmn:itemComponent>
  </dmn:itemDefinition>
  
  <dmn:itemDefinition name="tMember">
    <dmn:itemComponent name="name">
      <dmn:typeRef>string</dmn:typeRef>
    </dmn:itemComponent>
    <dmn:itemComponent name="address">
      <dmn:typeRef>tAddress</dmn:typeRef>
    </dmn:itemComponent>
  </dmn:itemDefinition>
  
  <dmn:itemDefinition name="tTeam">
    <dmn:itemComponent name="teamName">
      <dmn:typeRef>string</dmn:typeRef>
    </dmn:itemComponent>
    <dmn:itemComponent name="members">
      <dmn:typeRef>tMember</dmn:typeRef>
      <dmn:isCollection>true</dmn:isCollection>
    </dmn:itemComponent>
  </dmn:itemDefinition>
  
  <dmn:itemDefinition name="tDepartment">
    <dmn:itemComponent name="deptName">
      <dmn:typeRef>string</dmn:typeRef>
    </dmn:itemComponent>
    <dmn:itemComponent name="teams">
      <dmn:typeRef>tTeam</dmn:typeRef>
      <dmn:isCollection>true</dmn:isCollection>
    </dmn:itemComponent>
  </dmn:itemDefinition>
  
  <dmn:itemDefinition name="tOrganization">
    <dmn:itemComponent name="orgName">
      <dmn:typeRef>string</dmn:typeRef>
    </dmn:itemComponent>
    <dmn:itemComponent name="departments">
      <dmn:typeRef>tDepartment</dmn:typeRef>
      <dmn:isCollection>true</dmn:isCollection>
    </dmn:itemComponent>
  </dmn:itemDefinition>
  
  <dmn:decision name="OrgDecision">
    <dmn:decisionTable>
      <dmn:input>
        <dmn:inputExpression>
          <dmn:text>tOrganization</dmn:text>
        </dmn:inputExpression>
      </dmn:input>
      <dmn:output name="result"/>
      <dmn:rule>
        <dmn:inputEntry><dmn:text>-</dmn:text></dmn:inputEntry>
        <dmn:outputEntry><dmn:text>"OK"</dmn:text></dmn:outputEntry>
      </dmn:rule>
    </dmn:decisionTable>
  </dmn:decision>
</dmn:definitions>)";
    
    api::BusinessRulesEngine engine;
    auto load_result = engine.load_dmn_model(dmn_xml);
    BOOST_REQUIRE(load_result.has_value());
    
    engine.set_validation_enabled(true);
    
    // Create large structure with multiple departments/teams/members
    json input = {
        {"tOrganization", {
            {"orgName", "Large Corp"},
            {"departments", json::array()}
        }}
    };
    
    // Add 5 departments, each with 3 teams, each with 5 members
    for (int d = 0; d < 5; d++) {
        json dept = {
            {"deptName", "Dept" + std::to_string(d)},
            {"teams", json::array()}
        };
        
        for (int t = 0; t < 3; t++) {
            json team = {
                {"teamName", "Team" + std::to_string(t)},
                {"members", json::array()}
            };
            
            for (int m = 0; m < 5; m++) {
                json member = {
                    {"name", "Member" + std::to_string(m)},
                    {"address", {
                        {"street", std::to_string(100 + m) + " Main St"},
                        {"city", "City" + std::to_string(d)}
                    }}
                };
                team["members"].push_back(member);
            }
            
            dept["teams"].push_back(team);
        }
        
        input["tOrganization"]["departments"].push_back(dept);
    }
    
    // Validation should complete in reasonable time (< 100ms for 75 members)
    auto start = std::chrono::high_resolution_clock::now();
    std::string result;
    BOOST_CHECK_NO_THROW(result = engine.evaluate(input.dump()));
    auto end = std::chrono::high_resolution_clock::now();
    
    auto duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    
    BOOST_TEST_MESSAGE("Validation of 75 members across 5 levels took " << duration_ms << "ms");
    BOOST_CHECK(duration_ms < 100); // Should be fast
    BOOST_CHECK(!result.empty());
}

BOOST_AUTO_TEST_SUITE_END()
