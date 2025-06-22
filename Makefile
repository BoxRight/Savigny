# Makefile for Kelsen schema transpiler with context support
# Compiler and flags
CC = gcc
CFLAGS = -Wall -g -I. -I/usr/include/cjson

# Bison (parser generator)
YACC = bison
YFLAGS = -d -v

# Source files
SRCS = main.c schema_types.c config_validator.c custom_tokenizer.c context_manager.c
OBJS = $(SRCS:.c=.o) schema_parser.tab.o

# External dependencies
LIBS = -lcjson

# Output executable
TARGET = savigny

# Default target
all: $(TARGET)

# Link the executable
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

# Generate parser from Bison grammar
schema_parser.tab.c schema_parser.tab.h: schema_parser.y
	$(YACC) $(YFLAGS) $<

# Compile source files
%.o: %.c schema_parser.tab.h
	$(CC) $(CFLAGS) -c $< -o $@

# Clean up
clean:
	rm -f $(OBJS) schema_parser.tab.c schema_parser.tab.h schema_parser.output $(TARGET)

# Test run with context
test: $(TARGET)
	./$(TARGET) -v -c schema_config.json test_schema.txt

# Test run with context database
testcontext: $(TARGET)
	./$(TARGET) -v -c schema_config.json -x legal_context.json test_schema.txt

# Create documentation
docs:
	doxygen Doxyfile

.PHONY: all clean test testcontext docs

