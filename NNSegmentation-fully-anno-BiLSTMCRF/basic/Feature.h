/*
 * Feature.h
 *
 *  Created on: Mar 17, 2015
 *      Author: mszhang
 */

#ifndef SRC_FEATURE_H_
#define SRC_FEATURE_H_

#include <vector>

using namespace std;
class Feature {

public:
	vector<int> words;
	vector<int> chars;
	vector<int> tags;
	vector<int> linear_features;
	vector<string> glyphs;
	vector<vector<int> > spell_features;
public:
	Feature() {
	}
	virtual ~Feature() {

	}

	void clear() {
		words.clear();
		chars.clear();
		tags.clear();
		glyphs.clear();
		linear_features.clear();
                for (int i = 0;i < spell_features.size();i++){
                  spell_features[i].clear();
                }
		spell_features.clear();
	}
};

#endif /* SRC_FEATURE_H_ */
