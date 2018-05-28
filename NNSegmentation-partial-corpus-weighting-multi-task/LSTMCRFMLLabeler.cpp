/*
 * Labeler.cpp
 *
 *  Created on: Mar 16, 2015
 *      Author: mszhang
 */


#include "LSTMCRFMLLabeler.h"

#include "Argument_helper.h"

#include <sstream>

Labeler::Labeler() {
  nullkey = "-null-";
  unknownkey = "-unknown-";
  seperateKey = "#";

}

Labeler::~Labeler() {
  m_classifier.release();
}

int Labeler::createAlphabet(const vector<Instance>& vecInsts, const vector<size_t>& vecOtherInstsPosi, const string& othertrainFile) {
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
      			assert(tagNum == tagfeatures[iter_tag].size());
    		}

    		vector<string> features;
    		int curInstSize = labels.size();
    		int labelId;
    		for (int i = 0; i < curInstSize; ++i) {
			vector<string> vecCurLabels;
			split_bychar(labels[i], vecCurLabels,'/');
			for (int id_label = 0;id_label < vecCurLabels.size();id_label++){
				if (vecCurLabels[id_label].compare("A-SEG") == 0 or vecCurLabels[id_label].compare("a-seg") == 0)//如果label是A-SEG，那么就不考虑
					continue;
      				labelId = m_labelAlphabet.from_string(vecCurLabels[id_label]);

			}


      			string curword = normalize_to_lowerwithdigit(words[i]);
      			word_stat[curword]++;
      			for (int j = 0; j < sparsefeatures[i].size(); j++)
        			feature_stat[sparsefeatures[i][j]]++;
      			for (int j = 0; j < tagfeatures[i].size(); j++)
        			tag_stat[j][tagfeatures[i][j]]++;
    		}

    	/*	if ((numInstance + 1) % (500 * m_options.verboseIter) == 0) {
      			cout << numInstance + 1 << " ";
      			if ((numInstance + 1) % (2000 * m_options.verboseIter) == 0)
        			cout << std::endl;
      			cout.flush();
    		} */
    		if (m_options.maxInstance > 0 && numInstance == m_options.maxInstance)
      			break;
  	}
	


	if (othertrainFile != ""){
		assert(vecOtherInstsPosi.size() > 0);
		m_pipe.initInputFile(othertrainFile.c_str());
		size_t posi;
  		for (numInstance = 0; numInstance < vecOtherInstsPosi.size(); numInstance++) {
			posi = vecOtherInstsPosi[numInstance];
			Instance *pInstance = m_pipe.readInstanceByPosi(posi);
		
    		//const Instance *pInstance = &vecInsts[numInstance];

    		const vector<string> &words = pInstance->words;
    		const vector<string> &labels = pInstance->labels;
    		const vector<vector<string> > &sparsefeatures = pInstance->sparsefeatures;

			const vector<vector<string> > &tagfeatures = pInstance->tagfeatures;
    		for (int iter_tag = 0; iter_tag < tagfeatures.size(); iter_tag++) {
      			assert(tagNum == tagfeatures[iter_tag].size());
    		}

    		vector<string> features;
    		int curInstSize = labels.size();
    		int labelId;
    		for (int i = 0; i < curInstSize; ++i) {
			vector<string> vecCurLabels;
			split_bychar(labels[i], vecCurLabels,'/');
			for (int id_label = 0;id_label < vecCurLabels.size();id_label++){
				if (vecCurLabels[id_label].compare("A-SEG") == 0 or vecCurLabels[id_label].compare("a-seg") == 0)//如果label是A-SEG，那么就不考虑
					continue;
      				labelId = m_labelAlphabet.from_string(vecCurLabels[id_label]);

			}


      			string curword = normalize_to_lowerwithdigit(words[i]);
      			word_stat[curword]++;
      			for (int j = 0; j < sparsefeatures[i].size(); j++)
        			feature_stat[sparsefeatures[i][j]]++;
      			for (int j = 0; j < tagfeatures[i].size(); j++)
        			tag_stat[j][tagfeatures[i][j]]++;
    		}

    /*		if ((numInstance + 1) % (500 * m_options.verboseIter) == 0) {
      			cout << numInstance + 1 << " ";
      			if ((numInstance + 1) % (2000 * m_options.verboseIter) == 0)
        			cout << std::endl;
      			cout.flush();
    		}*/
    		if (m_options.maxInstance > 0 && numInstance == m_options.maxInstance)
      			break;
  	}
	m_pipe.uninitInputFile();
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
  
	//exit(0);
	cout << "Creating alphabet ending!!" << endl;
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

    /*		if ((numInstance + 1) % (500 * m_options.verboseIter) == 0) {
      			cout << numInstance + 1 << " ";
      			if ((numInstance + 1) % (2000 * m_options.verboseIter) == 0)
        			cout << std::endl;
      			cout.flush();
    		}*/
    		if (m_options.maxInstance > 0 && numInstance == m_options.maxInstance)
      			break;
  	}

  	//cout << numInstance << " " << endl;
}


void Labeler::train(const string& trainFile, const string& othertrainFile, const string& newsdevFile, const string& nonnewsdevFile, const string& newstestFile, const string& nonnewstestFile, const string& modelFile, const string& optionFile) {
  	if (optionFile != "")
    	m_options.load(optionFile);
  	m_options.showOptions();//配置文件

  
  	vector<Instance> trainInsts, newsdevInsts, nonnewsdevInsts, newstestInsts, nonnewstestInsts;
  	static vector<Instance> decodeInstResults;
  	static Instance curDecodeInst;
  	bool newsbCurIterBetter = false, nonnewsbCurIterBetter = false;

  	m_pipe.readInstances(trainFile, trainInsts, m_options.maxInstance);
  	if (newsdevFile != "")
    	m_pipe.readInstances(newsdevFile, newsdevInsts, m_options.maxInstance);
  	if (nonnewsdevFile != "")
    	m_pipe.readInstances(nonnewsdevFile, nonnewsdevInsts, m_options.maxInstance);
  	if (newstestFile != "")
    	m_pipe.readInstances(newstestFile, newstestInsts, m_options.maxInstance);
  	if (nonnewstestFile != "")
    	m_pipe.readInstances(nonnewstestFile, nonnewstestInsts, m_options.maxInstance);
	
	vector<size_t> othertrainInsts_posi; //局部标注数据是读取Insts的位置，而不是直接读到内存里面
	if (othertrainFile != "")
		m_pipe.readInstancesPosi(othertrainFile, othertrainInsts_posi, m_options.maxInstance);
	
	createAlphabet(trainInsts, othertrainInsts_posi, othertrainFile);

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

  	vector<Example> newsdevExamples, nonnewsdevExamples, newstestExamples, nonnewstestExamples;
  	initialExamples(newsdevInsts, newsdevExamples);
  	initialExamples(newstestInsts, newstestExamples);
  	initialExamples(nonnewsdevInsts, nonnewsdevExamples);
  	initialExamples(nonnewstestInsts, nonnewstestExamples);

  	dtype newsbestDIS = 0, nonnewsbestDIS = 0;
	
	int trainSampleSize, othertrainSampleSize;
	trainSampleSize = m_options.trainSampleSize;
	othertrainSampleSize = m_options.othertrainSampleSize;

	if (m_options.trainSampleSize < 1 || m_options.trainSampleSize > trainInsts.size())
		trainSampleSize = trainInsts.size();
	if (m_options.othertrainSampleSize < 1 || m_options.othertrainSampleSize > othertrainInsts_posi.size())
		othertrainSampleSize = othertrainInsts_posi.size();
	
	int inputSize = trainSampleSize + othertrainSampleSize;

	cout << endl;
	cout << "trainSampleSize: " << trainSampleSize << "\t\t" << "othertrainSampleSize: " << othertrainSampleSize << "\t\t" << "inputSize: " << inputSize << endl;
  	cout << endl;

	int batchBlock = inputSize / m_options.batchSize;
  	if (inputSize % m_options.batchSize != 0)
    		batchBlock++;

  	srand(0);
	std::vector<int> indexes, indexes_train, indexes_othertrain;
  	
	for (int i = 0; i < inputSize; ++i)
    	indexes.push_back(i);
	for (int i = 0 ; i < trainInsts.size(); ++i)
		indexes_train.push_back(i);
	for (int i = 0 ; i < othertrainInsts_posi.size(); ++i)
		indexes_othertrain.push_back(i);
	

  	static Metric eval;
	static Metric metric_news_dev, metric_nonnews_dev;
	static Metric metric_news_test, metric_nonnews_test;//这边可能要修改
  	
	static vector<Example> allSampleTrainExamples, subExamples;
	static vector<Instance> allSampleTrainInstances;
	
	time_t start_iter_time, end_iter_time;
  	
	int newsdevNum = newsdevExamples.size(), nonnewsdevNum = nonnewsdevExamples.size();
	int newstestNum = newstestExamples.size(), nonnewstestNum = nonnewstestExamples.size();

  	for (int iter = 0, stopIter = 0; iter < m_options.maxIter  && stopIter < m_options.stopIter; ++iter, ++stopIter) {
    	std::cout << "##### Iteration " << iter  << std::endl;
		time(&start_iter_time);
    	
		random_shuffle(indexes_train.begin(), indexes_train.end());
		random_shuffle(indexes_othertrain.begin(), indexes_othertrain.end());
		random_shuffle(indexes.begin(), indexes.end());
		
		allSampleTrainInstances.clear();
		allSampleTrainExamples.clear();
		
		for (int id_train = 0; id_train < trainSampleSize; id_train++)
			allSampleTrainInstances.push_back(trainInsts[indexes_train[id_train]]);
		
		size_t posi;
		if (othertrainFile != ""){
			m_pipe.initInputFile(othertrainFile.c_str());
			for (int id_otherTrain = 0; id_otherTrain < othertrainSampleSize; id_otherTrain++){
				posi = othertrainInsts_posi[indexes_othertrain[id_otherTrain]];
				Instance *pInstance = m_pipe.readInstanceByPosi(posi);
				Instance tmp_trainInstance;
            	tmp_trainInstance.copyValuesFrom(*pInstance);
				allSampleTrainInstances.push_back(tmp_trainInstance);
			}
			m_pipe.uninitInputFile();
		}
		
		assert(allSampleTrainInstances.size() == inputSize);
		
		initialExamples(allSampleTrainInstances, allSampleTrainExamples);
		allSampleTrainInstances.clear();
    		
		eval.reset();

    	for (int updateIter = 0; updateIter < batchBlock; updateIter++) {
      			subExamples.clear();
      			int start_pos = updateIter * m_options.batchSize;
      			int end_pos = (updateIter + 1) * m_options.batchSize;
      			if (end_pos > inputSize)
        			end_pos = inputSize;

      			for (int idy = start_pos; idy < end_pos; idy++) {
        			subExamples.push_back(allSampleTrainExamples[indexes[idy]]);
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
			
     			// m_classifier.updateParams(m_options.regParameter, m_options.adaAlpha, m_options.adaEps);
				m_classifier.updateParams(m_options.belta1, m_options.belta2, m_options.regParameter, m_options.adaAlpha, m_options.adaEps);
    	}	

    		if (newsdevNum > 0) {
      			newsbCurIterBetter = false;
      			if (!m_options.outBest.empty())
        			decodeInstResults.clear();
      			metric_news_dev.reset();
      			for (int idx = 0; idx < newsdevExamples.size(); idx++) {
        			vector<string> result_labels;
        			predict(newsdevExamples[idx].m_features, result_labels, newsdevInsts[idx].words);

        			if (m_options.seg)
          				newsdevInsts[idx].SegEvaluate(result_labels, metric_news_dev);
        			else
          				newsdevInsts[idx].Evaluate(result_labels, metric_news_test);

        			if (!m_options.outBest.empty()) {
          				curDecodeInst.copyValuesFrom(newsdevInsts[idx]);
          				curDecodeInst.assignLabel(result_labels);
          				decodeInstResults.push_back(curDecodeInst);
        			}
      			}	

				std::cout << "News dev:" << std::endl;
      			metric_news_dev.print();

      			if (!m_options.outBest.empty() && metric_news_dev.getAccuracy() > newsbestDIS) {
        			m_pipe.outputAllInstances(newsdevFile + m_options.outBest, decodeInstResults);
        			newsbCurIterBetter = true;
      			}

      			if (newstestNum > 0) {
        			if (!m_options.outBest.empty())
          				decodeInstResults.clear();
        			metric_news_test.reset();
        			for (int idx = 0; idx < newstestExamples.size(); idx++) {
          				vector<string> result_labels;
          				predict(newstestExamples[idx].m_features, result_labels, newstestInsts[idx].words);

          				if (m_options.seg)
            					newstestInsts[idx].SegEvaluate(result_labels, metric_news_test);
          				else
            					newstestInsts[idx].Evaluate(result_labels, metric_news_test);

          				if (newsbCurIterBetter && !m_options.outBest.empty()) {
            					curDecodeInst.copyValuesFrom(newstestInsts[idx]);
            					curDecodeInst.assignLabel(result_labels);
            					decodeInstResults.push_back(curDecodeInst);
          				}
        			}
        			std::cout << "News test:" << std::endl;
        			metric_news_test.print();

        			if (!m_options.outBest.empty() && newsbCurIterBetter) {
          				m_pipe.outputAllInstances(newstestFile + m_options.outBest, decodeInstResults);
        			}
      			}
      
      			if (m_options.saveIntermediate && metric_news_dev.getAccuracy() > newsbestDIS) {
					stopIter = 0;
        			std::cout << "News Exceeds best previous performance of " << newsbestDIS << ". Saving news model file.." << std::endl;
        			newsbestDIS = metric_news_dev.getAccuracy();
					stringstream ss;
					ss << iter;
					string s_updateIter = ss.str();
        			writeModelFile(modelFile + "news_dev=" + s_updateIter);
      			}

    		}
    		if (nonnewsdevNum > 0) {
      			nonnewsbCurIterBetter = false;
      			if (!m_options.outBest.empty())
        			decodeInstResults.clear();
      			metric_nonnews_dev.reset();
      			for (int idx = 0; idx < nonnewsdevExamples.size(); idx++) {
        			vector<string> result_labels;
        			predict(nonnewsdevExamples[idx].m_features, result_labels, nonnewsdevInsts[idx].words);

        			if (m_options.seg)
          				nonnewsdevInsts[idx].SegEvaluate(result_labels, metric_nonnews_dev);
        			else
          				nonnewsdevInsts[idx].Evaluate(result_labels, metric_nonnews_test);

        			if (!m_options.outBest.empty()) {
          				curDecodeInst.copyValuesFrom(nonnewsdevInsts[idx]);
          				curDecodeInst.assignLabel(result_labels);
          				decodeInstResults.push_back(curDecodeInst);
        			}
      			}	

				std::cout << "Non-News dev:" << std::endl;
      			metric_nonnews_dev.print();

      			if (!m_options.outBest.empty() && metric_nonnews_dev.getAccuracy() > nonnewsbestDIS) {
        			m_pipe.outputAllInstances(nonnewsdevFile + m_options.outBest, decodeInstResults);
        			nonnewsbCurIterBetter = true;
      			}

      			if (nonnewstestNum > 0) {
        			if (!m_options.outBest.empty())
          				decodeInstResults.clear();
        			metric_nonnews_test.reset();
        			for (int idx = 0; idx < nonnewstestExamples.size(); idx++) {
          				vector<string> result_labels;
          				predict(nonnewstestExamples[idx].m_features, result_labels, nonnewstestInsts[idx].words);

          				if (m_options.seg)
            					nonnewstestInsts[idx].SegEvaluate(result_labels, metric_nonnews_test);
          				else
            					nonnewstestInsts[idx].Evaluate(result_labels, metric_nonnews_test);

          				if (nonnewsbCurIterBetter && !m_options.outBest.empty()) {
            					curDecodeInst.copyValuesFrom(nonnewstestInsts[idx]);
            					curDecodeInst.assignLabel(result_labels);
            					decodeInstResults.push_back(curDecodeInst);
          				}
        			}
        			std::cout << "Non-News test:" << std::endl;
        			metric_nonnews_test.print();

        			if (!m_options.outBest.empty() && nonnewsbCurIterBetter) {
          				m_pipe.outputAllInstances(nonnewstestFile + m_options.outBest, decodeInstResults);
        			}
      			}
      
      			if (m_options.saveIntermediate && metric_nonnews_dev.getAccuracy() > nonnewsbestDIS) {
					stopIter = 0;
        			std::cout << "Non-News Exceeds best previous performance of " << nonnewsbestDIS << ". Saving non-news model file.." << std::endl;
        			nonnewsbestDIS = metric_nonnews_dev.getAccuracy();
					stringstream ss;
					ss << iter;
                    string s_updateIter = ss.str();
        			writeModelFile(modelFile + "nonnews_dev=" + s_updateIter);
      			}

    		}
    // Clear gradients
	time(&end_iter_time);
	//cost=difftime(end,start);  
	cout << "one iter difftime:\t" << difftime(end_iter_time, start_iter_time) << "(s)" << endl;
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

	std::string trainFile = "", othertrainFile = "";
 	std::string newsdevFile = "", nonnewsdevFile = ""; 
    std::string newstestFile = "", nonnewstestFile = ""; 
	std::string modelFile = "";
	std::string optionFile = "";
	std::string outputFile = "";
	bool bTrain = false;
	dsr::Argument_helper ah;

	ah.new_flag("l", "learn", "train or test", bTrain);
	ah.new_named_string("train", "trainCorpus", "named_string",
			"training corpus to train a model, must when training", trainFile);
	ah.new_named_string("othertrain", "othertrainCorpus", "named_string",
			"other training corpus to train a model, not must when training", othertrainFile);
	ah.new_named_string("newsdev", "newsdevCorpus", "named_string",
			"news development corpus to train a model, optional when training", newsdevFile);
	ah.new_named_string("nonnewsdev", "nonnewsdevCorpus", "named_string",
			"nonnews development corpus to train a model, optional when training", nonnewsdevFile);
	ah.new_named_string("newstest", "newstestCorpus", "named_string",
			"news testing corpus to train a model or input file to test a model, optional when training and must when testing", newstestFile);
	ah.new_named_string("nonnewstest", "nonnewstestCorpus", "named_string",
			"nonnews testing corpus to train a model or input file to test a model, optional when training and must when testing", nonnewstestFile);
	ah.new_named_string("model", "modelFile", "named_string",
			"model file, must when training and testing", modelFile);
	ah.new_named_string("option", "optionFile", "named_string",
			"option file to train a model, optional when training", optionFile);
	ah.new_named_string("output", "outputFile", "named_string",
			"output file to test, must when testing", outputFile);

	ah.process(argc, argv);

	Labeler tagger;
	if (bTrain) {
		tagger.train(trainFile, othertrainFile, newsdevFile, nonnewsdevFile, newstestFile, nonnewstestFile, modelFile, optionFile);

	} else {
		if (newstestFile != "")
			tagger.test(newstestFile, outputFile, modelFile );
		if (nonnewstestFile != "")
			tagger.test(nonnewstestFile, outputFile, modelFile);
	}

	//test(argv);
	//ah.write_values(std::cout);
#if USE_CUDA==1
	ShutdownTensorEngine();
#else
	ShutdownTensorEngine<cpu>();
#endif
}
