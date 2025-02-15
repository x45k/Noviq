#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>  // Add this for pow() function
#include "lexer_display.h"
#include "lexer_interpret.h"

// Remove duplicate type definitions since they're in lexar_interpret.h
Variable *variables = NULL;
size_t variableCount = 0;

// Add global line number definition
int currentLineNumber = 0;

Variable *findVariable(const char *name) {
    char *trimmed = strdup(name);
    char *ptr = trimmed;
    
    // Trim leading whitespace
    while (*ptr == ' ' || *ptr == '\t') ptr++;
    
    // Trim trailing whitespace
    char *end = ptr + strlen(ptr) - 1;
    while (end > ptr && (*end == ' ' || *end == '\t')) {
        *end = '\0';
        end--;
    }

    for (size_t i = 0; i < variableCount; i++) {
        if (strcmp(variables[i].name, ptr) == 0) {
            free(trimmed);
            return &variables[i];
        }
    }
    free(trimmed);
    return NULL;
}

void addVariable(const char *name, VarType type, void *value) {
    variables = realloc(variables, (variableCount + 1) * sizeof(Variable));
    variables[variableCount].name = strdup(name);
    variables[variableCount].type = type;
    if (type == INT) {
        variables[variableCount].value.intValue = *(int *)value;
    } else if (type == FLOAT) {
        variables[variableCount].value.floatValue = *(float *)value;
    } else if (type == BOOLEAN) {
        variables[variableCount].value.boolValue = *(int *)value;
    } else {
        variables[variableCount].value.stringValue = strdup((char *)value);
    }
    variableCount++;
}

// Add helper functions for float parsing
int isFloat(const char *str) {
    int dots = 0;
    int i = 0;

    // Skip leading minus sign
    if (str[0] == '-') {
        i++;
    }

    // String can't be just a minus sign
    if (str[i] == '\0') {
        return 0;
    }

    for (; str[i] != '\0'; i++) {
        if (str[i] == '.') {
            dots++;
        } else if (!isdigit(str[i])) {
            return 0;
        }
    }
    return dots == 1;
}

float parseFloat(const char *str) {
    return atof(str);
}

int isOperator(char c) {
    return c == '+' || c == '-' || c == '*' || c == '/' || c == '%';
}

int isLogicalOperator(const char *str) {
    return (strcmp(str, "AND") == 0 || 
            strcmp(str, "OR") == 0 || 
            strcmp(str, "NOT") == 0 ||
            strcmp(str, "&&") == 0 ||
            strcmp(str, "||") == 0 ||
            strcmp(str, "!") == 0);
}

int isComparisonOperator(const char *str) {
    return (strcmp(str, ">") == 0 || 
            strcmp(str, "<") == 0 || 
            strcmp(str, ">=") == 0 || 
            strcmp(str, "<=") == 0 || 
            strcmp(str, "==") == 0);
}

Variable performOperation(Variable *left, Variable *right, const char *operator) {
    Variable result;
    
    // Type checking
    if (left->type == STRING || right->type == STRING) {
        fprintf(stderr, "Error on line %d: Cannot perform arithmetic operations with strings\n", currentLineNumber);
        exit(EXIT_FAILURE);  // Changed from exit(1)
    }

    if (left->type == BOOLEAN || right->type == BOOLEAN) {
        fprintf(stderr, "Error on line %d: Cannot perform arithmetic operations with booleans\n", currentLineNumber);
        exit(EXIT_FAILURE);  // Changed from exit(1)
    }
    
    float leftVal = (left->type == FLOAT) ? left->value.floatValue : (float)left->value.intValue;
    float rightVal = (right->type == FLOAT) ? right->value.floatValue : (float)right->value.intValue;
    
    result.type = FLOAT;

    if (strcmp(operator, "**") == 0) {
        result.value.floatValue = pow(leftVal, rightVal);
    } else if (strcmp(operator, "//") == 0) {
        if (rightVal == 0) {
            fprintf(stderr, "Error on line %d: Division by zero\n", currentLineNumber);
            exit(EXIT_FAILURE);  // Changed from exit(1)
        }
        result.type = INT;
        result.value.intValue = (int)(leftVal / rightVal);
    } else if (strcmp(operator, "%") == 0) {
        if (rightVal == 0) {
            fprintf(stderr, "Error: Modulo by zero\n");
            exit(EXIT_FAILURE);
        }
        result.type = INT;
        result.value.intValue = (int)leftVal % (int)rightVal;
    } else {
        switch(operator[0]) {
            case '+': result.value.floatValue = leftVal + rightVal; break;
            case '-': result.value.floatValue = leftVal - rightVal; break;
            case '*': result.value.floatValue = leftVal * rightVal; break;
            case '/':
                if (rightVal == 0) {
                    fprintf(stderr, "Error: Division by zero\n");
                    exit(EXIT_FAILURE);
                }
                result.value.floatValue = leftVal / rightVal;
                break;
        }
    }

    // Convert to INT if result is a whole number and operation isn't division
    if (result.type == FLOAT && operator[0] != '/' && 
        result.value.floatValue == (int)result.value.floatValue) {
        result.type = INT;
        result.value.intValue = (int)result.value.floatValue;
    }
    
    return result;
}

Variable performLogicalOperation(Variable *left, Variable *right, const char *operator) {
    Variable result;
    result.type = BOOLEAN;

    // Convert operands to boolean values
    int leftBool = 0, rightBool = 0;

    if (left) {
        switch(left->type) {
            case BOOLEAN: leftBool = left->value.boolValue; break;
            case INT: leftBool = left->value.intValue != 0; break;
            case FLOAT: leftBool = left->value.floatValue != 0; break;
            case STRING: leftBool = strlen(left->value.stringValue) > 0; break;
        }
    }

    if (right) {
        switch(right->type) {
            case BOOLEAN: rightBool = right->value.boolValue; break;
            case INT: rightBool = right->value.intValue != 0; break;
            case FLOAT: rightBool = right->value.floatValue != 0; break;
            case STRING: rightBool = strlen(right->value.stringValue) > 0; break;
        }
    }

    if (strcmp(operator, "AND") == 0) {
        result.value.boolValue = leftBool && rightBool;
    } else if (strcmp(operator, "OR") == 0) {
        result.value.boolValue = leftBool || rightBool;
    } else if (strcmp(operator, "NOT") == 0) {
        result.value.boolValue = !leftBool;
    }

    return result;
}

Variable performComparison(Variable *left, Variable *right, const char *operator) {
    Variable result;
    result.type = BOOLEAN;

    // Convert operands to comparable values
    float leftVal, rightVal;

    switch(left->type) {
        case INT: leftVal = (float)left->value.intValue; break;
        case FLOAT: leftVal = left->value.floatValue; break;
        case BOOLEAN: leftVal = (float)left->value.boolValue; break;
        default:
            fprintf(stderr, "Error on line %d: Cannot compare string values\n", currentLineNumber);
            exit(EXIT_FAILURE);
    }

    switch(right->type) {
        case INT: rightVal = (float)right->value.intValue; break;
        case FLOAT: rightVal = right->value.floatValue; break;
        case BOOLEAN: rightVal = (float)right->value.boolValue; break;
        default:
            fprintf(stderr, "Error on line %d: Cannot compare string values\n", currentLineNumber);
            exit(EXIT_FAILURE);
    }

    if (strcmp(operator, ">") == 0) {
        result.value.boolValue = leftVal > rightVal;
    } else if (strcmp(operator, "<") == 0) {
        result.value.boolValue = leftVal < rightVal;
    } else if (strcmp(operator, ">=") == 0) {
        result.value.boolValue = leftVal >= rightVal;
    } else if (strcmp(operator, "<=") == 0) {
        result.value.boolValue = leftVal <= rightVal;
    } else if (strcmp(operator, "==") == 0) {
        result.value.boolValue = leftVal == rightVal;
    }

    return result;
}

Variable *evaluateExpression(const char *expr) {
    char *trimmed = strdup(expr);
    char *ptr = trimmed;
    
    while (*ptr == ' ' || *ptr == '\t') ptr++;

    // Handle single negative number directly
    if ((*ptr == '-' && isdigit(*(ptr + 1))) && !strchr(ptr + 1, '+') && 
        !strchr(ptr + 1, '*') && !strchr(ptr + 1, '/') && !strchr(ptr + 1, '%')) {
        Variable *result = malloc(sizeof(Variable));
        if (isFloat(ptr)) {
            result->type = FLOAT;
            result->value.floatValue = parseFloat(ptr);
        } else {
            result->type = INT;
            result->value.intValue = atoi(ptr);
        }
        free(trimmed);
        return result;
    }

    // First check for comparison operators
    char *gtOp = strstr(ptr, ">=");
    char *ltOp = strstr(ptr, "<=");
    char *eqOp = strstr(ptr, "==");
    char *gt = strstr(ptr, ">");
    char *lt = strstr(ptr, "<");
    
    char *op = NULL;
    const char *opStr = NULL;
    int opLen = 1;

    // Find the leftmost operator
    if (gtOp && (!op || gtOp < op)) { op = gtOp; opStr = ">="; opLen = 2; }
    if (ltOp && (!op || ltOp < op)) { op = ltOp; opStr = "<="; opLen = 2; }
    if (eqOp && (!op || eqOp < op)) { op = eqOp; opStr = "=="; opLen = 2; }
    if (gt && (!op || gt < op) && (gt != gtOp)) { op = gt; opStr = ">"; opLen = 1; }
    if (lt && (!op || lt < op) && (lt != ltOp)) { op = lt; opStr = "<"; opLen = 1; }

    if (op) {
        *op = '\0';
        char *leftStr = ptr;
        char *rightStr = op + opLen;

        Variable *left = evaluateExpression(leftStr);
        Variable *right = evaluateExpression(rightStr);

        if (left && right) {
            Variable *result = malloc(sizeof(Variable));
            *result = performComparison(left, right, opStr);
            free(left);
            free(right);
            free(trimmed);
            return result;
        }
        if (left) free(left);
        if (right) free(right);
        free(trimmed);
        return NULL;
    }

    // Check for NOT/! operator first
    if (strncmp(ptr, "NOT ", 4) == 0 || strncmp(ptr, "! ", 2) == 0 || ptr[0] == '!') {
        Variable *operand = evaluateExpression(ptr + (ptr[0] == 'N' ? 4 : (ptr[0] == '!' && ptr[1] == ' ' ? 2 : 1)));
        if (operand) {
            Variable *result = malloc(sizeof(Variable));
            *result = performLogicalOperation(operand, NULL, "NOT");
            free(operand);
            free(trimmed);
            return result;
        }
    }

    // Check for AND/OR/&&/|| operators
    char *andOp = strstr(ptr, " AND ");
    char *orOp = strstr(ptr, " OR ");
    char *andSymbolOp = strstr(ptr, "&&");
    char *orSymbolOp = strstr(ptr, "||");
    op = NULL;
    opStr = NULL;
    opLen = 0;

    // Find the leftmost operator
    if (andOp && (!op || andOp < op)) { op = andOp; opStr = "AND"; opLen = 5; }
    if (orOp && (!op || orOp < op)) { op = orOp; opStr = "OR"; opLen = 4; }
    if (andSymbolOp && (!op || andSymbolOp < op)) { op = andSymbolOp; opStr = "AND"; opLen = 2; }
    if (orSymbolOp && (!op || orSymbolOp < op)) { op = orSymbolOp; opStr = "OR"; opLen = 2; }

    if (op) {
        *op = '\0';
        char *leftStr = ptr;
        char *rightStr = op + opLen;

        // Trim spaces around operators
        while (*rightStr == ' ') rightStr++;

        Variable *left = evaluateExpression(leftStr);
        Variable *right = evaluateExpression(rightStr);

        if (left && right) {
            Variable *result = malloc(sizeof(Variable));
            *result = performLogicalOperation(left, right, opStr);
            free(left);
            free(right);
            free(trimmed);
            return result;
        }
        if (left) free(left);
        if (right) free(right);
        free(trimmed);
        return NULL;
    }

    // Check for two-character operators first
    op = NULL;
    for (char *c = ptr; *c; c++) {
        if ((c[0] == '*' && c[1] == '*') || 
            (c[0] == '/' && c[1] == '/')) {
            op = c;
            break;
        }
        // Check for modulo or single operators
        if (*c == '%' || (*c == '*' || *c == '/' || *c == '+' || *c == '-')) {
            if (*c == '-' && (c == ptr || isOperator(*(c-1)))) {
                continue;  // Skip leading minus or minus after operator
            }
            if (!op) op = c;
        }
    }

    if (!op) {
        // First check if it's a simple variable or value
        if (!strchr(ptr, '+') && !strchr(ptr, '/') && !strchr(ptr, '*')) {
            Variable *var = findVariable(ptr);
            if (var) {
                Variable *result = malloc(sizeof(Variable));
                *result = *var;
                free(trimmed);
                return result;
            }
            // Try parsing as number
            if (isdigit(*ptr) || (*ptr == '-' && isdigit(*(ptr + 1)))) {
                Variable *result = malloc(sizeof(Variable));
                if (isFloat(ptr)) {
                    result->type = FLOAT;
                    result->value.floatValue = parseFloat(ptr);
                } else {
                    result->type = INT;
                    result->value.intValue = atoi(ptr);
                }
                free(trimmed);
                return result;
            }
            free(trimmed);
            return NULL;
        }
    }

    // Handle two-character operators
    char operator[3] = {0};
    char *rightStr;
    if ((op[0] == '*' && op[1] == '*') || (op[0] == '/' && op[1] == '/')) {
        operator[0] = op[0];
        operator[1] = op[1];
        *op = '\0';
        rightStr = op + 2;
    } else {
        operator[0] = *op;
        *op = '\0';
        rightStr = op + 1;
    }
    char *leftStr = ptr;

    // Split the expression and save the operator
    char *left_trimmed = strdup(leftStr);
    char *right_trimmed = strdup(rightStr);
    
    char *left_ptr = left_trimmed;
    char *right_ptr = right_trimmed;
    
    // Trim left operand
    while (*left_ptr == ' ' || *left_ptr == '\t') left_ptr++;
    char *left_end = left_ptr + strlen(left_ptr) - 1;
    while (left_end > left_ptr && (*left_end == ' ' || *left_end == '\t')) {
        *left_end = '\0';
        left_end--;
    }
    
    // Trim right operand
    while (*right_ptr == ' ' || *right_ptr == '\t') right_ptr++;
    char *right_end = right_ptr + strlen(right_ptr) - 1;
    while (right_end > right_ptr && (*right_end == ' ' || *right_end == '\t')) {
        *right_end = '\0';
        right_end--;
    }

    // Get operands
    Variable *left = findVariable(left_ptr);
    Variable *right = findVariable(right_ptr);
    Variable temp_left, temp_right;

    // Handle numeric literals for left operand
    if (!left && (isdigit(*left_ptr) || (*left_ptr == '-' && isdigit(*(left_ptr + 1))))) {
        temp_left.type = isFloat(left_ptr) ? FLOAT : INT;
        if (temp_left.type == FLOAT)
            temp_left.value.floatValue = parseFloat(left_ptr);
        else
            temp_left.value.intValue = atoi(left_ptr);
        left = &temp_left;
    }

    // Handle numeric literals for right operand
    if (!right && (isdigit(*right_ptr) || (*right_ptr == '-' && isdigit(*(right_ptr + 1))))) {
        temp_right.type = isFloat(right_ptr) ? FLOAT : INT;
        if (temp_right.type == FLOAT)
            temp_right.value.floatValue = parseFloat(right_ptr);
        else
            temp_right.value.intValue = atoi(right_ptr);
        right = &temp_right;
    }

    // Clean up
    free(left_trimmed);
    free(right_trimmed);
    
    if (!left || !right) {
        free(trimmed);
        return NULL;
    }

    // Create operator string and perform operation
    Variable *result = malloc(sizeof(Variable));
    *result = performOperation(left, right, operator);

    free(trimmed);
    return result;
}

// Function to interpret and execute commands
void interpretCommand(const char *command, int lineNumber) {
    currentLineNumber = lineNumber;  // Set the current line number

    // Skip empty lines or lines with only whitespace
    const char *trimmed = command;
    while (*trimmed == ' ' || *trimmed == '\t') trimmed++;
    if (*trimmed == '\0') return;

    if (strncmp(trimmed, "import", 6) == 0) {
        char varName[256];
        char fileName[256];
        parseImportStatement(trimmed, varName, fileName);
        importVariableFromFile(fileName, varName);
        return;
    }

    // Handle display command
    if (strncmp(trimmed, "display(", 8) == 0) {
        const char *closingParenthesis = strchr(command + 8, ')');
        if (closingParenthesis) {
            char *content = (char *)malloc(closingParenthesis - (command + 8) + 1);
            strncpy(content, command + 8, closingParenthesis - (command + 8));
            content[closingParenthesis - (command + 8)] = '\0';

            if (strstr(content, "%var") != NULL && strchr(content, ',') != NULL) {
                char *format = strtok(content, ",");
                format[strlen(format) - 1] = '\0';
                format++;

                // Parse variables
                char *vars[10];  // Maximum 10 variables
                int varCount = 0;
                char *var;
                while ((var = strtok(NULL, ",")) != NULL) {
                    while (*var == ' ') var++;  // Skip leading spaces
                    vars[varCount++] = var;
                }

                displayFormatted(format, vars, varCount);
            } else {
                // Handle regular display cases
                char *endptr;
                long intValue = strtol(content, &endptr, 10);
                if (*endptr == '\0') {
                    displayInt((int)intValue);
                } else if (isFloat(content)) {
                    displayFloat(parseFloat(content));
                } else if ((content[0] == '"' && content[strlen(content) - 1] == '"') ||
                          (content[0] == '\'' && content[strlen(content) - 1] == '\'')) {
                    content[strlen(content) - 1] = '\0';
                    display(content + 1);
                } else {
                    printf("Syntax error on line %d: invalid format\n", lineNumber);
                    exit(EXIT_FAILURE);
                }
            }
            free(content);
        } else {
            printf("Syntax error on line %d: missing closing parenthesis\n", lineNumber);
            exit(EXIT_FAILURE);
        }
    } else if (strchr(trimmed, '=') != NULL) {
        char *equalsSign = strchr(command, '=');
        size_t nameLength = equalsSign - command;
        char *name = (char *)malloc(nameLength + 1);
        strncpy(name, command, nameLength);
        name[nameLength] = '\0';

        // Trim trailing whitespace from variable name
        char *end = name + nameLength - 1;
        while (end > name && (*end == ' ' || *end == '\t')) {
            *end = '\0';
            end--;
        }

        // Trim leading whitespace from variable name
        char *start = name;
        while (*start == ' ' || *start == '\t') start++;
        if (start != name) {
            memmove(name, start, strlen(start) + 1);
        }

        char *value = equalsSign + 1;
        while (*value == ' ') value++;

        // Check for arithmetic expression first
        Variable *result = evaluateExpression(value);
        if (result) {
            if (result->type == FLOAT) {
                float floatVal = result->value.floatValue;
                updateVariable(name, FLOAT, &floatVal);
            } else if (result->type == INT) {
                int intVal = result->value.intValue;
                updateVariable(name, INT, &intVal);
            } else if (result->type == BOOLEAN) {
                int boolVal = result->value.boolValue;
                updateVariable(name, BOOLEAN, &boolVal);
            }
            free(result);
        } else {
            // Handle non-expression assignments
            if (strcmp(value, "true") == 0) {
                int boolValue = 1;
                updateVariable(name, BOOLEAN, &boolValue);
            } else if (strcmp(value, "false") == 0) {
                int boolValue = 0;
                updateVariable(name, BOOLEAN, &boolValue);
            } else if (isdigit(value[0]) || (value[0] == '-' && isdigit(value[1]))) {
                if (isFloat(value)) {
                    float floatValue = parseFloat(value);
                    updateVariable(name, FLOAT, &floatValue);
                } else {
                    int intValue = atoi(value);
                    updateVariable(name, INT, &intValue);
                }
            } else if (value[0] == '"' || value[0] == '\'') {
                size_t valueLength = strlen(value);
                value[valueLength - 1] = '\0';
                updateVariable(name, STRING, value + 1);
            }
        }

        free(name);
    } else {
        printf("Unknown command on line %d: %s\n", currentLineNumber, trimmed);
        exit(EXIT_FAILURE);
    }
}

// Add implementation for control flow handling
int evaluateCondition(const char *condition) {
    Variable *result = evaluateExpression(condition);
    if (!result) return 0;
    
    int value;
    if (result->type == BOOLEAN) {
        value = result->value.boolValue;
    } else if (result->type == INT) {
        value = result->value.intValue != 0;
    } else if (result->type == FLOAT) {
        value = result->value.floatValue != 0;
    } else if (result->type == STRING) {
        value = strlen(result->value.stringValue) > 0;
    } else {
        value = 0;
    }
    
    free(result);
    return value;
}

typedef struct {
    int indentLevel;
    int lastConditionMet;
    int inControlBlock;
    int skipNextBlock;  // Add flag to skip blocks after condition is met
    int nestingLevel;
} BlockState;

// Update BlockState initialization
BlockState blockState = {
    .indentLevel = 0,
    .lastConditionMet = 0,
    .inControlBlock = 0,
    .skipNextBlock = 0,
    .nestingLevel = 0
};

void executeBlock(const char *line, FILE *file) {
    char *nextLine = NULL;
    size_t len = 0;
    ssize_t read;
    int currentNestLevel = blockState.nestingLevel;

    read = getline(&nextLine, &len, file);
    if (read == -1) {
        free(nextLine);
        return;
    }
    currentLineNumber++;  // Increment line counter for each line read

    // Count initial indentation
    int baseIndent = 0;
    while (nextLine[baseIndent] == ' ' || nextLine[baseIndent] == '\t') baseIndent++;

    // Process first line
    if (nextLine[read-1] == '\n') nextLine[read-1] = '\0';
    if (!blockState.skipNextBlock) {
        // Check if this is another control statement
        if (strncmp(nextLine + baseIndent, "if(", 3) == 0 ||
            strncmp(nextLine + baseIndent, "elseif(", 7) == 0 ||
            strcmp(nextLine + baseIndent, "else:") == 0) {
            blockState.nestingLevel++;
            handleIfStatement(nextLine + baseIndent, file);
        } else {
            interpretCommand(nextLine + baseIndent, currentLineNumber);
        }
    }
    free(nextLine);
    nextLine = NULL;

    while ((read = getline(&nextLine, &len, file)) != -1) {
        currentLineNumber++;  // Increment line counter for each line read
        
        if (nextLine[read-1] == '\n') nextLine[read-1] = '\0';
        
        int indent = 0;
        while (nextLine[indent] == ' ' || nextLine[indent] == '\t') indent++;
        
        if (indent < baseIndent) {
            ungetc('\n', file);
            for (int i = strlen(nextLine) - 1; i >= 0; i--) {
                ungetc(nextLine[i], file);
            }
            currentLineNumber--;  // Decrement because we're putting this line back
            break;
        }
        
        if (indent == baseIndent && !blockState.skipNextBlock) {
            char *line = nextLine + indent;
            if (strncmp(line, "if(", 3) == 0 ||
                strncmp(line, "elseif(", 7) == 0 ||
                strcmp(line, "else:") == 0) {
                blockState.nestingLevel++;
                handleIfStatement(line, file);
            } else {
                interpretCommand(line, currentLineNumber);
            }
        }
    }
    
    blockState.nestingLevel = currentNestLevel;
    free(nextLine);
}

void handleIfStatement(const char *line, FILE *file) {
    char condition[256];
    int currentNestLevel = blockState.nestingLevel;
    
    if (strncmp(line, "if(", 3) == 0) {
        if (sscanf(line, "if(%[^)]):", condition) != 1) {
            fprintf(stderr, "Error: Invalid if statement syntax\n");
            return;
        }
        blockState.inControlBlock = 1;
        blockState.lastConditionMet = evaluateCondition(condition);
        blockState.skipNextBlock = !blockState.lastConditionMet;
        executeBlock(line, file);
    }
    else if (strncmp(line, "elseif(", 7) == 0) {
        if (!blockState.inControlBlock) {
            fprintf(stderr, "Error: elseif without if\n");
            return;
        }
        if (blockState.lastConditionMet) {
            blockState.skipNextBlock = 1;
        } else {
            if (sscanf(line, "elseif(%[^)]):", condition) == 1) {
                blockState.lastConditionMet = evaluateCondition(condition);
                blockState.skipNextBlock = !blockState.lastConditionMet;
            }
        }
        executeBlock(line, file);
    }
    else if (strcmp(line, "else:") == 0) {
        if (!blockState.inControlBlock) {
            fprintf(stderr, "Error: else without if\n");
            return;
        }
        blockState.skipNextBlock = blockState.lastConditionMet;
        executeBlock(line, file);
        if (currentNestLevel == 0) {
            blockState.inControlBlock = 0;
            blockState.lastConditionMet = 0;
        }
    }
}

void updateVariable(const char *name, VarType type, void *value) {
    for (size_t i = 0; i < variableCount; i++) {
        if (strcmp(variables[i].name, name) == 0) {
            // Free existing string if needed
            if (variables[i].type == STRING && variables[i].value.stringValue) {
                free(variables[i].value.stringValue);
            }
            
            // Update type and value
            variables[i].type = type;
            if (type == STRING) {
                variables[i].value.stringValue = strdup((char *)value);
            } else if (type == INT) {
                variables[i].value.intValue = *(int *)value;
            } else if (type == FLOAT) {
                variables[i].value.floatValue = *(float *)value;
            } else if (type == BOOLEAN) {
                variables[i].value.boolValue = *(int *)value;
            }
            return;
        }
    }
    
    // If variable not found, add it
    addVariable(name, type, value);
}

void parseImportStatement(const char *line, char *varName, char *fileName) {
    // example: "import var from "file.nvq""
    sscanf(line, "import %s from \"%[^\"]\"", varName, fileName);
}

void importVariableFromFile(const char *fileName, const char *varName) {
    FILE *file = fopen(fileName, "r");
    if (!file) {
        fprintf(stderr, "Error: Could not open file %s\n", fileName);
        exit(EXIT_FAILURE);
    }

    char line[256];
    while (fgets(line, sizeof(line), file)) {
        char name[256];
        char value[256];
        if (sscanf(line, "%s = %[^\n]", name, value) == 2) {
            if (strcmp(name, varName) == 0) {
                if (value[0] == '"' && value[strlen(value) - 1] == '"') {
                    value[strlen(value) - 1] = '\0';
                    updateVariable(varName, STRING, value + 1);
                } else if (isFloat(value)) {
                    float floatValue = parseFloat(value);
                    updateVariable(varName, FLOAT, &floatValue);
                } else if (isdigit(value[0]) || (value[0] == '-' && isdigit(value[1]))) {
                    int intValue = atoi(value);
                    updateVariable(varName, INT, &intValue);
                } else if (strcmp(value, "true") == 0 || strcmp(value, "false") == 0) {
                    int boolValue = (strcmp(value, "true") == 0) ? 1 : 0;
                    updateVariable(varName, BOOLEAN, &boolValue);
                }
                break;
            }
        }
    }

    fclose(file);
}