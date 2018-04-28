/*
 * Labeler.cpp
 *
 *  Created on: Mar 16, 2015
 *      Author: mszhang
 */


#include "LSTMCRFMLLabeler.h"

#include "Argument_helper.h"

Labeler::Labeler() {
  nullkey = "-null-";
  unknownkey = "-unknown-";
  seperateKey = "#";

}

Labeler::~Labeler() {
  m_classifier.release();
}

int Labeler::createAlphabet(const vector<Instance>& vecInsts) {
  	cout << "Creating Alphabet..." << endl;

  	int numInstance;
  	hash_map<string, int> feature_stat;
  	hash_map<string, int> word_stat;
  	vector<hash_map<string, int> > tag_stat;
  	m_labelAlphabet.clear();

  	int tagNum = vecInsts[0].tagfeatures[0].size();
  	tag_stat.resize(tagNum);
  	m_tagAlphabets.resize(tagNum);

  	for (numInstance = 0; numInstance < vecInsts.size(); numInstance++) {
    		const Instance *pInstance = &vecInsts[numInstance];

    		const vector<string> &words = pInstance->words;
    		const vector<string> &labels = pInstance->labels;
    		const vector<vector<string> > &sparsefeatures = pInstance->sparsefeatures;

		const vector<vector<string> > &tagfeatures = pInstance->tagfeatures;
    		for (int iter_tag = 0; iter_tag < tagfeatures.size(); iter_tag++) {
      			//cout << tagNum << tagfeatures[iter_tag].size() << endl;
			//cout << words[iter_tag] << endl;
			assert(tagNum == tagfeatures[iter_tag].size());
    		}

    		vector<string> features;
    		int curInstSize = labels.size();
    		int labelId;
    		for (int i = 0; i < curInstSize; ++i) {
			vector<string> vecCurLabels;
		//	cout << labels[i] << ": ";
			split_bychar(labels[i], vecCurLabels,'/');
			for (int id_label = 0;id_label < vecCurLabels.size();id_label++){
		//		cout << vecCurLabels[id_label] << "&&";
				if (vecCurLabels[id_label].compare("A-SEG") == 0 or vecCurLabels[id_label].compare("a-seg") == 0)//如果label是A-SEG，那么就不考虑
					continue;
      				labelId = m_labelAlphabet.from_string(vecCurLabels[id_label]);

			}
			//cout << endl;


      			string curword = normalize_to_lowerwithdigit(words[i]);
      			word_stat[curword]++;
      			for (int j = 0; j < sparsefeatures[i].size(); j++)
        			feature_stat[sparsefeatures[i][j]]++;
      			for (int j = 0; j < tagfeatures[i].size(); j++)
        			tag_stat[j][tagfeatures[i][j]]++;
    		}

    		if ((numInstance + 1) % m_options.verboseIter == 0) {
      			cout << numInstance + 1 << " ";
      			if ((numInstance + 1) % (40 * m_options.verboseIter) == 0)
        			cout << std::endl;
      			cout.flush();
    		}
    		if (m_options.maxInstance > 0 && numInstance == m_options.maxInstance)
      			break;
  		}

  	cout << numInstance << " " << endl;
  	cout << "Label num: " << m_labelAlphabet.size() << endl;
  	cout << "Total word num: " << word_stat.size() << endl;
  	cout << "Total feature num: " << feature_stat.size() << endl;
	// tag print information
  	cout << "tag num = " << tagNum << endl;
  	for (int iter_tag = 0; iter_tag < tagNum; iter_tag++) {
    		cout << "Total tag " << iter_tag << " num: " << tag_stat[iter_tag].size() << endl;
  	}
  	m_featAlphabet.clear();
  	m_wordAlphabet.clear();
 	m_wordAlphabet.from_string(nullkey);
  	m_wordAlphabet.from_string(unknownkey);

  	//tag apheabet init
  	for (int i = 0; i < tagNum; i++) {
    		m_tagAlphabets[i].clear();
    		m_tagAlphabets[i].from_string(nullkey);
    		m_tagAlphabets[i].from_string(unknownkey);
  	}

  	hash_map<string, int>::iterator feat_iter;
  	for (feat_iter = feature_stat.begin(); feat_iter != feature_stat.end(); feat_iter++) {
    		if (feat_iter->second > m_options.featCutOff) {
      			m_featAlphabet.from_string(feat_iter->first);
    		}
  	}

  	for (feat_iter = word_stat.begin(); feat_iter != word_stat.end(); feat_iter++) {
   		if (!m_options.wordEmbFineTune || feat_iter->second > m_options.wordCutOff) {
      			m_wordAlphabet.from_string(feat_iter->first);
    		}
  	}

  	cout << "before tag alphabet line 121" << endl;
	// tag cut off, default tagCutOff is zero
  	for (int i = 0; i < tagNum; i++) {
    		for (feat_iter = tag_stat[i].begin(); feat_iter != tag_stat[i].end(); feat_iter++) {
      			if (!m_options.tagEmbFineTune || feat_iter->second > m_options.tagCutOff) {
        			m_tagAlphabets[i].from_string(feat_iter->first);
      			}
    		}
  	}

  	cout << "Remain feature num: " << m_featAlphabet.size() << endl;
  	cout << "Remain words num: " << m_wordAlphabet.size() << endl;
	// tag Remain num print
  	for (int i = 0; i < tagNum; i++) {
    		cout << "Remain tag " << i << " num: " << m_tagAlphabets[i].size() << endl;
  	}

  	m_labelAlphabet.set_fixed_flag(true);
  	m_featAlphabet.set_fixed_flag(true);
  	m_wordAlphabet.set_fixed_flag(true);

	// tag Alphabet fixed  
  	for (int iter_tag = 0; iter_tag < tagNum; iter_tag++) {
    		m_tagAlphabets[iter_tag].set_fixed_flag(true);
  	}
  

  	return 0;
}



void Labeler::extractFeature(Feature& feat, const Instance* pInstance, int idx) {
  	feat.clear();

  	const vector<string>& words = pInstance->words;
  	int sentsize = words.size();
  	string curWord = idx >= 0 && idx < sentsize ? normalize_to_lowerwithdigit(words[idx]) : nullkey;

  	// word features
  	int unknownId = m_wordAlphabet.from_string(unknownkey);

  	int curWordId = m_wordAlphabet.from_string(curWord);
  	if (curWordId >= 0)
    		feat.words.push_back(curWordId);
  	else
    		feat.words.push_back(unknownId);

  	// tag features
  	const vector<vector<string> > &tagfeatures = pInstance->tagfeatures;
  	int tagNum = tagfeatures[idx].size();
  	for (int i = 0; i < tagNum; i++) {
    		unknownId = m_tagAlphabets[i].from_string(unknownkey);
    		int curTagId = m_tagAlphabets[i].from_string(tagfeatures[idx][i]);
    		if (curTagId >= 0)
      			feat.tags.push_back(curTagId);
    		else
      			feat.tags.push_back(unknownId);
  	}

  	const vector<string>& linear_features = pInstance->sparsefeatures[idx];
  	for (int i = 0; i < linear_features.size(); i++) {
    		int curFeatId = m_featAlphabet.from_string(linear_features[i]);
    		if (curFeatId >= 0)
      			feat.linear_features.push_back(curFeatId);
 	}

}

void Labeler::convert2Example(const Instance* pInstance, Example& exam) {
  	exam.clear();
  	const vector<string> &labels = pInstance->labels;
  	int curInstSize = labels.size();
  	for (int i = 0; i < curInstSize; ++i) {
		int numLabel1s = m_labelAlphabet.size();
    		string orcale = labels[i];
    		vector<int> curlabels;
		if (orcale.compare("a-seg") == 0 || orcale.compare("A-SEG") == 0){
			for (int j = 0; j < numLabel1s; ++j) {
				curlabels.push_back(1);
			}
			
		} 
		else{
			for (int j = 0; j < numLabel1s; ++j)
				curlabels.push_back(0);
			vector<string> vecCurLabels;
			split_bychar(orcale, vecCurLabels,'/');
			for (int id_label = 0; id_label < vecCurLabels.size(); id_label ++){
    				for (int j = 0; j < numLabel1s; ++j) {
      					string str = m_labelAlphabet.from_id(j);
      					if (str.compare(vecCurLabels[id_label]) == 0)
        					curlabels[j] = 1;
    				}
			}
		}
/*		cout << "labels:" << orcale << endl;
		cout << "curlabels " ;
		for (int tmp_ix = 0; tmp_ix < curlabels.size();tmp_ix ++){
			cout << curlabels[tmp_ix] << " " ;
		}
		cout << endl;*/
    		exam.m_labels.push_back(curlabels);
    		Feature feat;
    		extractFeature(feat, pInstance, i);
    		exam.m_features.push_back(feat);   
  	}
//	exit(0);
}

void Labeler::initialExamples(const vector<Instance>& vecInsts, vector<Example>& vecExams) {
  	int numInstance;
  	for (numInstance = 0; numInstance < vecInsts.size(); numInstance++) {
    		const Instance *pInstance = &vecInsts[numInstance];
    		Example curExam;
    		convert2Example(pInstance, curExam);
    		vecExams.push_back(curExam);

    		if ((numInstance + 1) % m_options.verboseIter == 0) {
      			cout << numInstance + 1 << " ";
      			if ((numInstance + 1) % (40 * m_options.verboseIter) == 0)
        			cout << std::endl;
      			cout.flush();
    		}
    		if (m_options.maxInstance > 0 && numInstance == m_options.maxInstance)
      			break;
  	}

  	cout << numInstance << " " << endl;
}


void Labeler::train(const string& trainFile, const string& devFile, const string& testFile, const string& modelFile, const string& optionFile) {
  	if (optionFile != "")
    		m_options.load(optionFile);
  	m_options.showOptions();//配置文件

  
  	vector<Instance> trainInsts, devInsts, testInsts;
  	static vector<Instance> decodeInstResults;
  	static Instance curDecodeInst;
  	bool bCurIterBetter = false;

  	m_pipe.readInstances(trainFile, trainInsts, m_options.maxInstance);
  	if (devFile != "")
    	m_pipe.readInstances(devFile, devInsts, m_options.maxInstance);
  	if (testFile != "")
    	m_pipe.readInstances(testFile, testInsts, m_options.maxInstance);

  	createAlphabet(trainInsts);//统计词频以及一些特征的频率，并创建对应的字典
	//exit(0);

  	NRMat<dtype> wordEmb;
  	wordEmb.resize(m_wordAlphabet.size(), m_options.wordEmbSize);
  	wordEmb.randu(1000);

  	NRVec<NRMat<dtype> > tagEmbs(m_tagAlphabets.size());
  	for (int idx = 0; idx < tagEmbs.size(); idx++){
    		tagEmbs[idx].resize(m_tagAlphabets[idx].size(), m_options.tagEmbSize);
    		tagEmbs[idx].randu(1002 + idx);
  	}
  
  	m_classifier.setWordEmbFinetune(m_options.wordEmbFineTune);
  	m_classifier.init(wordEmb, m_options.wordcontext, tagEmbs, m_labelAlphabet.size(), m_options.rnnHiddenSize, m_options.hiddenSize);
  	m_classifier.setTagEmbFinetune(m_options.tagEmbFineTune);
  	m_classifier.setDropValue(m_options.dropProb);

  	vector<Example> trainExamples, devExamples, testExamples;
  	initialExamples(trainInsts, trainExamples);
  	initialExamples(devInsts, devExamples);
  	initialExamples(testInsts, testExamples);

  	dtype bestDIS = 0;

  	int inputSize = trainExamples.size();

  	int batchBlock = inputSize / m_options.batchSize;
  	if (inputSize % m_options.batchSize != 0)
    		batchBlock++;

  	srand(0);
  	std::vector<int> indexes;
  	for (int i = 0; i < inputSize; ++i)
    		indexes.push_back(i);

  	static Metric eval, metric_dev, metric_test;
  	static vector<Example> subExamples;
  	int devNum = devExamples.size(), testNum = testExamples.size();
  	for (int iter = 0; iter < m_options.maxIter; ++iter) {
    		std::cout << "##### Iteration " << iter << std::endl;

    		random_shuffle(indexes.begin(), indexes.end());
    		eval.reset();
    		for (int updateIter = 0; updateIter < batchBlock; updateIter++) {
      			subExamples.clear();
      			int start_pos = updateIter * m_options.batchSize;
      			int end_pos = (updateIter + 1) * m_options.batchSize;
      			if (end_pos > inputSize)
        			end_pos = inputSize;

      			for (int idy = start_pos; idy < end_pos; idy++) {
        			subExamples.push_back(trainExamples[indexes[idy]]);
      			}

      			int curUpdateIter = iter * batchBlock + updateIter;
      			dtype cost = m_classifier.process(subExamples, curUpdateIter);

      			eval.overall_label_count += m_classifier._eval.overall_label_count;
      			eval.correct_label_count += m_classifier._eval.correct_label_count;

      			if ((curUpdateIter + 1) % m_options.verboseIter == 0) {
        			//m_classifier.checkgrads(subExamples, curUpdateIter+1);
        			std::cout << "current: " << updateIter + 1 << ", total block: " << batchBlock << std::endl;
        			std::cout << "Cost = " << cost << ", Tag Correct(%) = " << eval.getAccuracy() << std::endl;
      			}	
			//exit(0);
     			// m_classifier.updateParams(m_options.regParameter, m_options.adaAlpha, m_options.adaEps);
			m_classifier.updateParams(m_options.belta1, m_options.belta2, m_options.regParameter, m_options.adaAlpha, m_options.adaEps);
    		}	

    		if (devNum > 0) {
      			bCurIterBetter = false;
      			if (!m_options.outBest.empty())
        			decodeInstResults.clear();
      			metric_dev.reset();
      			for (int idx = 0; idx < devExamples.size(); idx++) {
        			vector<string> result_labels;
        			predict(devExamples[idx].m_features, result_labels, devInsts[idx].words);

        			if (m_options.seg)
          				devInsts[idx].SegEvaluate(result_labels, metric_dev);
        			else
          				devInsts[idx].Evaluate(result_labels, metric_dev);

        			if (!m_options.outBest.empty()) {
          				curDecodeInst.copyValuesFrom(devInsts[idx]);
          				curDecodeInst.assignLabel(result_labels);
          				decodeInstResults.push_back(curDecodeInst);
        			}
      			}	

      			metric_dev.print();

      			if (!m_options.outBest.empty() && metric_dev.getAccuracy() > bestDIS) {
        			m_pipe.outputAllInstances(devFile + m_options.outBest, decodeInstResults);
        			bCurIterBetter = true;
      			}

      			if (testNum > 0) {
        			if (!m_options.outBest.empty())
          				decodeInstResults.clear();
        			metric_test.reset();
        			for (int idx = 0; idx < testExamples.size(); idx++) {
          				vector<string> result_labels;
          				predict(testExamples[idx].m_features, result_labels, testInsts[idx].words);

          				if (m_options.seg)
            					testInsts[idx].SegEvaluate(result_labels, metric_test);
          				else
            					testInsts[idx].Evaluate(result_labels, metric_test);

          				if (bCurIterBetter && !m_options.outBest.empty()) {
            					curDecodeInst.copyValuesFrom(testInsts[idx]);
            					curDecodeInst.assignLabel(result_labels);
            					decodeInstResults.push_back(curDecodeInst);
          				}
        			}
        			std::cout << "test:" << std::endl;
        			metric_test.print();

        			if (!m_options.outBest.empty() && bCurIterBetter) {
          				m_pipe.outputAllInstances(testFile + m_options.outBest, decodeInstResults);
        			}
      			}
      
      			if (m_options.saveIntermediate && metric_dev.getAccuracy() > bestDIS) {
        			std::cout << "Exceeds best previous performance of " << bestDIS << ". Saving model file.." << std::endl;
        			bestDIS = metric_dev.getAccuracy();
        			writeModelFile(modelFile);
      			}

    		}
    // Clear gradients
  	}
}

int Labeler::predict(const vector<Feature>& features, vector<string>& outputs, const vector<string>& words) {
  assert(features.size() == words.size());
  vector<int> labelIdx, label2Idx;
  m_classifier.predict(features, labelIdx);
  outputs.clear();

  for (int idx = 0; idx < words.size(); idx++) {
    string label = m_labelAlphabet.from_id(labelIdx[idx]);
    outputs.push_back(label);
  }

  return 0;
}

void Labeler::test(const string& testFile, const string& outputFile, const string& modelFile) {
  loadModelFile(modelFile);
  vector<Instance> testInsts;
  m_pipe.readInstances(testFile, testInsts);

  vector<Example> testExamples;
  initialExamples(testInsts, testExamples);

  int testNum = testExamples.size();
  vector<Instance> testInstResults;
  Metric metric_test;
  metric_test.reset();
  for (int idx = 0; idx < testExamples.size(); idx++) {
    vector<string> result_labels;
    predict(testExamples[idx].m_features, result_labels, testInsts[idx].words);
    testInsts[idx].SegEvaluate(result_labels, metric_test);
    Instance curResultInst;
    curResultInst.copyValuesFrom(testInsts[idx]);
    testInstResults.push_back(curResultInst);
  }
  std::cout << "test:" << std::endl;
  metric_test.print();

  m_pipe.outputAllInstances(outputFile, testInstResults);

}

void Labeler::loadModelFile(const string& inputModelFile) {

}

void Labeler::writeModelFile(const string& outputModelFile) {

}

int main(int argc, char* argv[]) {
#if USE_CUDA==1
	InitTensorEngine();
#else
	InitTensorEngine<cpu>();
#endif

	std::string trainFile = "", devFile = "", testFile = "", modelFile = "";
	std::string optionFile = "";
	std::string outputFile = "";
	bool bTrain = false;
	dsr::Argument_helper ah;

	ah.new_flag("l", "learn", "train or test", bTrain);
	ah.new_named_string("train", "trainCorpus", "named_string",
			"training corpus to train a model, must when training", trainFile);
	ah.new_named_string("dev", "devCorpus", "named_string",
			"development corpus to train a model, optional when training",
			devFile);
	ah.new_named_string("test", "testCorpus", "named_string",
			"testing corpus to train a model or input file to test a model, optional when training and must when testing",
			testFile);
	ah.new_named_string("model", "modelFile", "named_string",
			"model file, must when training and testing", modelFile);
	ah.new_named_string("option", "optionFile", "named_string",
			"option file to train a model, optional when training", optionFile);
	ah.new_named_string("output", "outputFile", "named_string",
			"output file to test, must when testing", outputFile);

	ah.process(argc, argv);

	Labeler tagger;
	if (bTrain) {
		tagger.train(trainFile, devFile, testFile, modelFile, optionFile);
	} else {
		tagger.test(testFile, outputFile, modelFile);
	}

	//test(argv);
	//ah.write_values(std::cout);
#if USE_CUDA==1
	ShutdownTensorEngine();
#else
	ShutdownTensorEngine<cpu>();
#endif
}
