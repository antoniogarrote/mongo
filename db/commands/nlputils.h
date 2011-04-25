// nlputils.h

/**
 *
 *    This program is free software: you can redistribute it and/or  modify
 *    it under the terms of the GNU Affero General Public License, version 3,
 *    as published by the Free Software Foundation.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU Affero General Public License for more details.
 *
 *    You should have received a copy of the GNU Affero General Public License
 *    along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include "pch.h"

#define min(a, b) (((a) < (b)) ? (a) : (b))
#define max(a, b) (((a) > (b)) ? (a) : (b))

// Auxiliar functions
namespace {

  int _min(int a, int b, int c) {
    return min(min(a, b), c);
  }

  int **create_matrix(int Row, int Col) {
    int **array = new int*[Row];
    for(int i = 0; i < Row; ++i) {
      array[i] = new int[Col];
    }
    return array;
  }

  int **delete_matrix(int **array, int Row, int Col) {
    for(int i = 0; i < Row; ++i) {
      delete array[i];
    }
    delete [] array;
    return array;
  }

  int *create_matrix_row(int Col) {
    int *array = new int[Col];
    return array;
  }

  void delete_matrix_row(int *array) {
    delete [] array;
  }

}

namespace mongo {

  namespace nlputils {
    
    // Splits a string into tokens using the provided delimiters
    void tokenize(const string& str, vector<string>& tokens, const string& delimiters = " .,;:");

    // Computes the max similarity possible between two string using the levenshtein distance due to their length
    template <class TokensCollection> float  maxSimilarity(TokensCollection& x, TokensCollection& y) {
      float xSize = (float) x.size();
      float ySize = (float) y.size();
      float similarity;
      if(ySize > xSize) {
        similarity = 1 - ((ySize - xSize) / ySize);
      } else {
        similarity = 1 - ((xSize - ySize) / xSize);
      }

      return similarity;
    };

    // computes the Levenshtein distance between two collection of tokens
    template <class TokensCollection> int levenshteinDistance(TokensCollection& x, TokensCollection& y) {
      unsigned int m = (unsigned int) x.size();
      unsigned int n = (unsigned int) y.size();

      if (n == 0) {
        return m;
      } 
      else if (m == 0) {
        return n;
      }

      int *matrix_a = create_matrix_row(n + 1);
      int *matrix_b = create_matrix_row(n + 1);
      unsigned int i = 0;
      unsigned int j = 0;

      for(i = 0; i <= n; ++i) {
        matrix_a[i] = i; 
      }

      unsigned int up;
      unsigned int left;
      unsigned int upleft;
      unsigned int cost;


      int *current_matrix = matrix_a;
      int *prev_matrix    = matrix_b;

      for(i = 1; i <= m; ++i) {

        // swap rows
        if(current_matrix == matrix_a) {
          current_matrix = matrix_b;
          prev_matrix    = matrix_a;
        } else {
          current_matrix = matrix_a;
          prev_matrix    = matrix_b;
        }
        current_matrix[0] = i;

        for(j = 1; j <= n; ++j) {

          if (x[i-1] == y[j-1]) {
            cost = 0;
          } 
          else {
            cost = 1;
          }


          upleft = prev_matrix[j-1];
          up = prev_matrix[j];
          left = current_matrix[j-1];

          current_matrix[j] = _min(up + 1, left + 1, upleft + cost);
        }
      }

      unsigned int result = current_matrix[n];

      delete_matrix_row(current_matrix);
      delete_matrix_row(prev_matrix);
      
      return result;
    };

  }

}
