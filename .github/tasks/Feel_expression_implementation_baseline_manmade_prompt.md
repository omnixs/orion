# Objective
Implement all missing FEEL features from the expressions as defined in chapters 9 and 10 D:\Orion\docs\formal-24-01-01.pdf.

# Sources
You have access to the following sources:
- The documentation of the FEEL features/expressions and MDM 
- An overview of missing FEEL features/expressions in D:/Orion/.github/tasks/Implemented_so_far.md
- The prompts in D:/Orion/.github/prompts, which can be used to create propper sub-agents.
- The instructions and agents in D:/Orion/.github/instructions and D:/Orion/.github/agents, respectively.

# Task
The ultimate task is to implement the FEEL expressions as defined in chapters 9 and 10 D:\Orion\docs\formal-24-01-01.pdf. To achieve this, first create a list of all FEEL expressions and features that need to be implemented. Then, create a sub-task for each feature/expression or group of related features/expressions. For each sub-task ensure the following:
- A new sub-agent is assigned to this subtask, this agent is based on D:/Orion/.github/prompts/add_dmn_feature.md.
- The task is defined to perform the following tasks iteratively:
    1. generate code
    2. fix bugs using D:/Orion/.github/prompts/fix_bug.md
    3. improve the performance using D:/Orion/.github/prompts/improve_perf.md
    4. improve the quality using D:/Orion/.github/prompts/improve_quality.md
    5. test the new code to ensure the feature/expression is implemented correctly.
    6. test the all code to ensure no regression occures on other parts
    7. repeat steps 1-6 until the generated code passes all tests in steps 5. and 6. is of sufficient quality, performance.

Once all sub-tasks are completed, test the final generated codebase and makesure that all FEEL expressions and features have been implemented successfully. If there are missing expressions or features. Create new sub-task for these and repeat the plan outlined above untill everything is implemented correctly.