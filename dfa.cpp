#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <map>
#include <set>

const std::string ALPHABET    = ".ALPHABET";
const std::string STATES      = ".STATES";
const std::string TRANSITIONS = ".TRANSITIONS";
const std::string INPUT       = ".INPUT";
const std::string EMPTY       = ".EMPTY";

bool isChar(std::string s) {
  return s.length() == 1;
}
bool isRange(std::string s) {
  return s.length() == 3 && s[1] == '-';
}

// Locations in the program that you should modify to store the
// DFA information have been marked with four-slash comments:
//// (Four-slash comment)

int main() {
  std::istream& in = std::cin;
  std::string s;
  
  // Data structures to store DFA
  std::set<char> alphabet;
  std::string initialState;
  std::set<std::string> acceptingStates;
  std::map<std::pair<std::string, char>, std::string> transitions;

  std::getline(in, s); // Alphabet section (skip header)
  // Read characters or ranges separated by whitespace
  while(in >> s) {
    if (s == STATES) { 
      break; 
    } else {
      if (isChar(s)) {
        //// Variable 's[0]' is an alphabet symbol
        alphabet.insert(s[0]);
      } else if (isRange(s)) {
        for(char c = s[0]; c <= s[2]; ++c) {
          //// Variable 'c' is an alphabet symbol
          alphabet.insert(c);
        }
      } 
    }
  }

  std::getline(in, s); // States section (skip header)
  // Read states separated by whitespace
  while(in >> s) {
    if (s == TRANSITIONS) { 
      break; 
    } else {
      static bool initial = true;
      bool accepting = false;
      if (s.back() == '!' && !isChar(s)) {
        accepting = true;
        s.pop_back();
      }
      //// Variable 's' contains the name of a state
      if (initial) {
        //// The state is initial
        initialState = s;
        initial = false;
      }
      if (accepting) {
        //// The state is accepting
        acceptingStates.insert(s);
      }
    }
  }

  std::getline(in, s); // Transitions section (skip header)
  // Read transitions line-by-line
  while(std::getline(in, s)) {
    if (s == INPUT) { 
      // Note: Since we're reading line by line, once we encounter the
      // input header, we will already be on the line after the header
      break; 
    } else {
      std::string fromState, symbols, toState;
      std::istringstream line(s);
      std::vector<std::string> lineVec;
      while(line >> s) {
        lineVec.push_back(s);
      }
      fromState = lineVec.front();
      toState = lineVec.back();
      for(int i = 1; i < lineVec.size()-1; ++i) {
        std::string s = lineVec[i];
        if (isChar(s)) {
          symbols += s;
        } else if (isRange(s)) {
          for(char c = s[0]; c <= s[2]; ++c) {
            symbols += c;
          }
        }
      }
      for ( char c : symbols ) {
        //// There is a transition from 'fromState' to 'toState' on 'c'
        transitions[{fromState, c}] = toState;
      }
    }
  }

  // Input section (already skipped header)
  while (in >> s) {
    //// Variable 's' contains an input string for the DFA
    // Handling empty string
    if (s == EMPTY) {
      s = "";
    }
    
    std::string currentState = initialState;
    bool accepted = true;
    
    // Process each character in the input string store it as pair in transition
    for (char c : s) {
      auto key = std::make_pair(currentState, c);
      if (transitions.find(key) != transitions.end()) {
        currentState = transitions[key];
      } else {
        // No transition exists for this character
        accepted = false;
        break;
      }
    }
    
    // Check if we ended in an accepting state
    if (accepted && acceptingStates.find(currentState) != acceptingStates.end()) {
      std::cout << (s.empty() ? EMPTY : s) << " true" << std::endl;
    } else {
      std::cout << (s.empty() ? EMPTY : s) << " false" << std::endl;
    }
  }
}
