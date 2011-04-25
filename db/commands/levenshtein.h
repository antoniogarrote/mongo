// tmMatches.h

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

namespace mongo {

  namespace levenshtein {

    /**
     * holds config information
     */
    class Config {

    public:

      string dbname;
      string ns;
      string field;
      string sourceTerm;
      double threshold;
      bool   isWord;
      bool   isSentence;
      string separators;
      bool   hasLimit;
      int    limit;
      bool   hasOutput;
      string outputField;

      Config( const string& _dbname , const BSONObj& cmdObj );
    };

  }

}
