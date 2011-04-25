// levenshtein.cpp

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



#include "pch.h"
#include "../commands.h"
#include "../../client/dbclient.h"
#include "../../client/connpool.h"
#include "../../client/parallel.h"
#include "../replpair.h"
#include <math.h>

#include "levenshtein.h"
#include "nlputils.h"

namespace mongo {

  namespace levenshtein {

    Config::Config( const string& _dbname , const BSONObj& cmdObj ) {

      dbname = _dbname;
      ns = dbname + "." + cmdObj.firstElement().valuestr();

      // passing arguments
      sourceTerm = cmdObj["sourceTerm"].String();
      threshold  = cmdObj["threshold"].Double();
      isWord     = cmdObj["word"].Bool();
      isSentence = cmdObj["sentence"].Bool();
      separators = cmdObj["separators"].String();
      field      = cmdObj["field"].String();
      hasLimit   = cmdObj.hasField("limit");
      if(hasLimit) {
        limit = cmdObj["limit"].numberInt();
      }
      hasOutput   = cmdObj.hasField("outputField");
      if(hasOutput) {
        outputField = cmdObj["outputField"].String(); 
      }
    }

    class LevenshteinCommand : public Command {

    public:
      LevenshteinCommand() : Command("levenshtein", false, "levenshtein") {}
      
      virtual bool slaveOk() const { return !replSet; }
      virtual bool slaveOverrideOk() { return true; }

      virtual void help( stringstream &help ) const {
        help << "Find matches in a db collection using the lvenshtein distance and the provided threshold.\n";
      }

      virtual LockType locktype() const { return NONE; }

      bool run(const string& dbname , BSONObj& cmd, string& errmsg, BSONObjBuilder& result, bool fromRepl ) {
        // Configuration
        Client::GodScope cg;
        Config config( dbname , cmd );

        // traversing collection
        readlock lock( config.ns );        
        Client::Context ctx( config.ns );

        shared_ptr<Cursor> temp = bestGuessCursor( config.ns.c_str(), BSONObj(), BSONObj() );        
        auto_ptr<ClientCursor> cursor( new ClientCursor( QueryOption_NoCursorTimeout , temp , config.ns.c_str() ) );

        int counter = 0;

        vector<BSONObj> scores;
        vector<string> tokens;

        if(config.isSentence) {
          mongo::nlputils::tokenize(config.sourceTerm, tokens, config.separators);
        }

        while ( cursor->ok() ) {


          // moving the cursor
          if ( cursor->currentIsDup() ) {
            cursor->advance();
            continue;
          }

          if ( ! cursor->currentMatches() ) {
            cursor->advance();
            continue;
          }

          BSONObj o = cursor->current();
          cursor->advance();


          // extracting the text and computing the score

          string tmTerm = o[config.field].String();
          int dist=0; 
          float metric;
          float maxSimilarity;

          if(config.isSentence) {
            vector<string> tmTokens;
            mongo::nlputils::tokenize(tmTerm,tmTokens, config.separators);

            // we skip if we cannot get the min threshold rquired
            maxSimilarity = mongo::nlputils::maxSimilarity(tmTokens,tokens);
            if(maxSimilarity>config.threshold) {
              if(tokens.size() > tmTokens.size()) {
                dist = mongo::nlputils::levenshteinDistance < vector<string> > (tmTokens, 
                                                                                tokens);
              } else {
                dist = mongo::nlputils::levenshteinDistance < vector<string> > (tokens,tmTokens);
              }
              metric = 1 - ((float) dist / (float) max(tmTokens.size(),tokens.size()));
            } else {
              metric = 0;
            }
            
          } else {
            int sourceLength = config.sourceTerm.length();
            int tmTermLength = tmTerm.length();

            // we skip if we cannot get the min threshold rquired
            maxSimilarity = mongo::nlputils::maxSimilarity(config.sourceTerm,tmTerm);
            if(maxSimilarity>config.threshold) {            
              if(tmTerm.size() > config.sourceTerm.size()) {
                dist = mongo::nlputils::levenshteinDistance <string> (config.sourceTerm,tmTerm);
              } else {
                dist = mongo::nlputils::levenshteinDistance <string> (tmTerm, 
                                                                      config.sourceTerm);
              }
              metric = 1 - ((float) dist / (float) max(tmTermLength,sourceLength));
            } else {
              metric = 0;
            }
          }

          if(metric >= config.threshold) {
            BSONObjBuilder score;
            if(config.hasOutput) {
              //score.append( "match", o[config.outputField] );
              extractValue(o[config.outputField], score);
            } else {
              score.append( "match", o );
            }
            score.append( "score", metric );
            score.append( "distance", dist );
            //score.append( "maxSimilarity", maxSimilarity);
            scores.push_back( score.obj() );
            
            counter++;
          }

          if(config.hasLimit && (config.limit == counter)) {
            break;
          }
        }

        // Building output
        result.append("results", scores);
        result.append("hits", counter);
        if(config.isSentence) {
          result.append("level", "sentence");
        } else {
          result.append("level", "word");
        }

        return true;
      };

    protected:
      
      void extractValue(BSONElement elem, BSONObjBuilder &match)  {
        BSONType t = elem.type();

        switch ( t ) {
        case MinKey:
          match.append("match",elem.numberInt());
          break;
        case MaxKey:
          match.append("match",elem.numberLong());
          break;
        case NumberDouble:
          match.append("match",elem.Double());
          break;
        case NumberInt:
          match.append("match",elem.Int());
          break;
        case NumberLong:
          match.append("match",elem.Long());
          break;
        case mongo::String:
          match.append("match",elem.String());
          break;
        case Object:
          match.append("match", elem.value());
          break;
        case mongo::Array:
          match.append("match",elem.Array());
          break;            
        case jstOID:
          match.append("match",elem.OID());
          break;            
        case mongo::Bool:
          match.append("match",elem.Bool());
          break;            
        case mongo::Date:
        case Timestamp:
          match.append("match",elem.Date());
          break;            
        case mongo::EOO:
        case mongo::BinData:
        case mongo::Undefined:
        case mongo::jstNULL:
        case mongo::RegEx:
        case mongo::DBRef:
        case mongo::Code:
        case mongo::Symbol:
        case mongo::CodeWScope:
          break;
        }
      };

    } levenshteinCommand; // end of LevenshteinCommand class
  }
}
