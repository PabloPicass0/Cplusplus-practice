#include <iostream>
#include <fstream>
#include <cassert>
#include <cstring>
#include <cctype>
#include <cstdlib>
#include <vector>

#include "common.h"
#include "mask.h"
#include "gogen.h"

using namespace std;

/* You are pre-supplied with the functions below. Add your own 
   function definitions to the end of this file. */

/* internal helper function which allocates a dynamic 2D array */
char **allocate_2D_array(int rows, int columns) {
  char **m = new char *[rows];
  assert(m);
  for (int r=0; r<rows; r++) {
    m[r] = new char[columns];
    assert(m[r]);
  }
  return m;
}

/* internal helper function which deallocates a dynamic 2D array */
void deallocate_2D_array(char **m, int rows) {
  for (int r=0; r<rows; r++)
    delete [] m[r];
  delete [] m;
}

/* internal helper function which removes unprintable characters like carriage returns and newlines from strings */
void filter (char *line) {
  while (*line) {
    if (!isprint(*line))
     *line = '\0';
    line++;
  }
}

/* loads a Gogen board from a file */
char **load_board(const char *filename) {
  char **board = allocate_2D_array(5, 6);
  ifstream input(filename);
  assert(input);
  char buffer[512];
  int lines = 0;
  input.getline(buffer, 512);
  while (input && lines < HEIGHT) {
    filter(buffer);
    if (strlen(buffer) != WIDTH)
      cout << "WARNING bad input = [" << buffer << "]" << endl;
    assert(strlen(buffer) == WIDTH);
    strcpy(board[lines], buffer);
    input.getline(buffer, 512);
    lines++;
  }
  input.close();
  return board;
}

/* saves a Gogen board to a file */
bool save_board(char **board, const char *filename) {
  ofstream out(filename); 
  if (!out)
    return false;
  for (int r=0; r<HEIGHT; r++) {
    for (int c=0; c<WIDTH; c++) {
      out << board[r][c];
    }
    out << endl;
  }
  bool result = out.good();
  out.close();
  return result;
}

/* internal helper function for counting number of words in a file */
int count_words(const char *filename) {
  char word[512];
  int count = 0;
  ifstream in(filename);
  while (in >> word)
    count++;
  in.close();
  return count;
}

/* loads a word list from a file into a NULL-terminated array of char *'s */
char **load_words(const char *filename) {
  int count = count_words(filename);
  ifstream in(filename);
  assert(in);
  int n=0;
  char **buffer = new char *[count+1]; // +1 because we NULL terminate 
  char word[512];
  for (; (in >> word) && n<count; n++) {
    buffer[n] = new char[strlen(word) + 1];
    strcpy(buffer[n], word);
  }
  buffer[n] = NULL;
  in.close();
  return buffer;
}

/* prints a Gogen board in appropriate format */
void print_board(char **board) {
  for (int r=0; r<HEIGHT; r++) {
    for (int c=0; c<WIDTH; c++) {
      cout << "[" << board[r][c] << "]";
      if (c < WIDTH-1)
	cout << "--";
    }
    cout <<endl;
    if (r < HEIGHT-1) {
      cout << " | \\/ | \\/ | \\/ | \\/ |" << endl;
      cout << " | /\\ | /\\ | /\\ | /\\ |" << endl;
    }
  }
}

/* prints a NULL-terminated list of words */
void print_words(char **words) {
  for (int n=0; words[n]; n++) 
    cout << words[n] << endl;
}

/* frees up the memory allocated in load_board(...) */
void delete_board(char **board) {
  deallocate_2D_array(board, HEIGHT);
}

/* frees up the memory allocated in load_words(...) */
void delete_words(char **words) {
  int count = 0;
  for (; words[count]; count++);
  deallocate_2D_array(words, count);
}

/* add your own function definitions here */

//sets row and col of a char in a board, or returns false and sets both to -1
bool get_position(char** board, char ch, int& row, int& column) {

  //go row by row
  for (int r = 0; r < HEIGHT; r++) {
    for (int c = 0; c < WIDTH; c++) {
      //check if char found
      if (ch == board[r][c]) {
	row = r;
        column = c;
	return true;
      }
    }
  }
  //otherwise return false
  row = column = -1;
  return false;
}


//returns true if a board is a solution to provided words, assuming the words contain all letters
bool valid_solution(char** board, char**  words) {

  //solution is valid, if all words are in there, with each char being one cell apart
  //each letter A-Y must be in there once

  //declare needed indices
  int row_a, row_b, col_a, col_b;

  //check word by word
  for (int i = 0; words[i]; i++) {
    //letter by letter
    for (int n = 0; n < strlen(words[i]) - 1; n++) {

      //check if chars of a word are only one position apart and on the board
      bool pos_1 = get_position(board, words[i][n], row_a, col_a);
      bool pos_2 = get_position(board, words[i][n + 1], row_b, col_b);

      if (!pos_1 || !pos_2 ||abs(row_a - row_b) > 1 || abs(col_a - col_b) > 1) {
	return false;
      }

      //check if Z is in there
      if (words[i][n] == 'Z' || words[i][n + 1] == 'Z') {
	return false;
      }
    }
  }
  return true;
}


//updates mutually board and mask depending on ch
void update(char** board, const char ch, Mask& mask) {

  int row(-3), col(-3);

  //if ch is found on the board, set mask to false with exception of r and c
  if (get_position(board, ch, row, col)) {
    mask.set_all(false);
    mask.set_element(row, col, true);
  }

  //else if not found, for every cell on the board that is not ch, set mask to false
  else if (!get_position(board, ch, row, col)) {
    for (int r = 0; r < HEIGHT; r++) {
      for (int c = 0; c < WIDTH; c++) {
	if (isalpha(board[r][c])) {
      	  mask[r][c] = false;
	}
      }
    }
  }

  //else if there is only one cell in mask true, put that ch into the board
  if (mask.count() == 1) {
    mask.get_position(true, row, col);
    board[row][col] = ch;
  }
}


//interesects two masks with d-neighborhood to one another
void neighbourhood_intersect(Mask& one, Mask& two) {

  //create nghborhood masks
  Mask nbr_one = one.neighbourhood();
  Mask nbr_two = two.neighbourhood();

  //alter masks
  one.intersect_with(nbr_two);
  two.intersect_with(nbr_one);
}

//helper that updates masks and board for adjacent letters in a word
void refine(char** board, char* word, int i, Mask masks[25]) {

  //when word is over, return
  if (i == strlen(word) - 1) {
    return;
  }

  //perform intersection
  neighbourhood_intersect(masks[word[i]], masks[word[i + 1]]);

  //update masks and board
  for (int n = 0; n < 25; n++) {
    update(board, (char) n + 'A', masks[n]); 
  }

  //do next letter in word
  refine(board, word, i + 1, masks); 
}


//attempts to solve a board and returns it or returns false if no solution found
bool solve_board(char** board, char** words) {
  
  //create array of masks
  Mask masks[25];
  for (char l = 'A'; l <= 'Z'; l++) {
    Mask m;
    update(board, l, m);
    masks[l - 'A'] = m;
  }

  //for a limit of iterations of 3
  for (int i = 0; i < 4; i++) {
    //intersect masks of adjacent letters and update board for each word
    for (int n = 0; words[n]; n++) {
      refine(board, words[n], 0, masks);
    }
    //if board is complete, return
    print_board(board);
    cout << i << endl;
    if (valid_solution(board, words)) {
      return true;
    }
  }

  //bruteforce last missing letteres
}
