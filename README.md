# Word-Seg based BiLSTMCRF

#### introduction 
- xxxxxLabeler.cpp 里面包含所有的外部信息，创建词典等等
- basic/ 分类器的具体实现，模型的初始化
- LibN3L/ 张梅山老师的库
- example/存放代码的配置文件，语料




#### command to run
- cd xx   (代码目录)
- ./cleanall.sh
- cmake . -DMKL=1
- make LSTMCRFMLLabeler -j 10
- cd example/
- vi run.sh  (修改代码路径，以及语料)
- ./run.sh


#### fully && partial
- 全标注语料格式：    char bichar tagfully
- 局部标注语料格式：  char bichar tagpartial
- tag:
	- tagfully:	B-SEG	M-SEG	E-SEG	S-SEG	
	- tagpartial:	B-SEG	M-SEG	E-SEG	S-SEG	A-SEG(四种可能)	B-SEG/S-SEG(可能输b或者s标签)	etc
