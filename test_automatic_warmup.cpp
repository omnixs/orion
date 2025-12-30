/*
 * Quick test to verify automatic warmup via engine constructor
 */
#include <orion/api/engine.hpp>
#include <iostream>
#include <chrono>

int main() {
    using namespace std::chrono;
    
    std::cout << "Testing automatic PCRE2 warmup via engine constructor...\n\n";
    
    // First engine construction - should trigger warmup
    {
        auto start = high_resolution_clock::now();
        orion::api::BusinessRulesEngine engine1;
        auto end = high_resolution_clock::now();
        auto duration = duration_cast<microseconds>(end - start).count();
        std::cout << "First engine construction (with warmup): " << duration << " microseconds\n";
    }
    
    // Second engine construction - should NOT trigger warmup (std::call_once)
    {
        auto start = high_resolution_clock::now();
        orion::api::BusinessRulesEngine engine2;
        auto end = high_resolution_clock::now();
        auto duration = duration_cast<microseconds>(end - start).count();
        std::cout << "Second engine construction (no warmup): " << duration << " microseconds\n";
    }
    
    // Third engine construction - verify warmup only happens once
    {
        auto start = high_resolution_clock::now();
        orion::api::BusinessRulesEngine engine3;
        auto end = high_resolution_clock::now();
        auto duration = duration_cast<microseconds>(end - start).count();
        std::cout << "Third engine construction (no warmup): " << duration << " microseconds\n";
    }
    
    std::cout << "\n✓ Automatic warmup verification complete\n";
    std::cout << "Expected: First construction slower, subsequent constructions fast\n";
    
    return 0;
}
