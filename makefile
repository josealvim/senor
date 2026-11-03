.POSIX:

CXX = c++
CXXFLAGS = -std=c++20 -Wall -Wextra -Wpedantic -Werror -I..
LDFLAGS =
# Prefer depi from PATH; fall back to the sibling checkout.
DEPI = depi
DEPI_FALLBACK = ../depi/depi
DEPI_FLAGS = -q -I=..
MAP = --map='tests/%.cpp=tests/%.o'
SRCS = tests/test.cpp
OBJS = tests/test.o
TARGET = tests/test
DEPFILE = .depi.mk

all: test

test: $(TARGET)
	./$(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(LDFLAGS) -o $@ $(OBJS)

tests/test.o: tests/test.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(DEPFILE): $(SRCS)
	@tmp="$@.$$$$"; \
	trap 'rm -f "$$tmp"' 0; \
	trap 'exit 1' 1 2 15; \
	depi='$(DEPI)'; \
	if ! command -v "$$depi" >/dev/null 2>&1 && [ ! -x "$$depi" ]; then \
		depi='$(DEPI_FALLBACK)'; \
	fi; \
	if ! command -v "$$depi" >/dev/null 2>&1 && [ ! -x "$$depi" ]; then \
		echo "depi not found; set DEPI to the depi binary" >&2; \
		exit 1; \
	fi; \
	printf '%s\n' $(SRCS) | \
	    "$$depi" --depfile="$@" $(MAP) $(DEPI_FLAGS) - > "$$tmp" && \
	mv "$$tmp" "$@"

include $(DEPFILE)

clean:
	rm -f $(TARGET) $(OBJS) $(DEPFILE)

.PHONY: all test clean
