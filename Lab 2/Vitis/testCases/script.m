A = [160,129,104,71,184,193,55,86;
39,31,145,150,32,65,90,45;
153,159,93,24,149,136,108,61;
133,73,183,124,151,39,68,67;
91,24,115,144,125,0,255,42;
148,92,126,71,175,204,100,156;
170,149,168,58,198,192,48,86;
91,79,47,91,104,71,146,75;
64,73,151,145,78,41,78,39;
96,12,126,98,177,40,118,78;
26,108,116,129,0,85,54,92;
135,167,172,73,161,159,58,64;
116,104,171,124,132,166,108,72;
57,64,157,177,83,71,156,22;
146,14,127,144,151,94,0,45;
138,111,101,84,175,136,48,103;
137,140,111,45,157,255,98,75;
161,156,172,124,170,189,110,125;
69,64,77,98,92,178,136,67;
151,181,181,82,175,152,95,89;
104,90,177,106,121,128,85,89;
61,25,71,117,129,53,63,45;
255,34,255,236,192,36,60,136;
95,2,118,98,124,85,115,50;
166,219,156,82,253,225,43,120;
92,184,107,84,120,153,83,67;
43,41,119,98,57,55,68,117;
137,117,144,131,147,165,88,83;
117,73,127,91,142,149,33,92;
89,111,129,137,210,63,75,86;
83,60,214,124,60,49,136,92;
140,19,141,137,162,57,15,33;
86,12,114,157,83,94,93,106;
156,225,129,98,170,207,141,97;
118,18,111,137,138,70,43,42;
100,138,116,71,143,201,88,72;
148,41,47,58,173,140,108,22;
177,159,141,11,184,251,141,58;
96,0,88,111,122,65,173,45;
149,226,137,112,162,172,100,100;
160,214,149,61,200,214,90,117;
136,116,197,116,170,191,141,103;
124,141,107,8,143,146,90,83;
73,86,81,55,74,88,93,184;
64,40,130,161,63,76,161,56;
58,43,131,157,51,136,38,42;
180,167,145,45,255,188,80,114;
117,135,101,65,120,137,123,78;
83,6,79,111,116,80,43,61;
137,23,116,98,161,85,183,50;
152,184,183,157,175,182,125,86;
120,110,203,65,138,152,100,111;
191,255,153,78,203,213,136,106;
0,44,0,0,81,90,90,50;
83,21,48,71,78,135,125,45;
118,73,164,99,122,90,125,78;
123,175,183,117,170,184,141,173;
111,185,149,53,129,226,83,72;
176,184,156,80,212,181,50,106;
146,183,186,124,157,220,95,139;
96,120,115,111,129,97,118,255;
126,200,181,86,175,185,110,125;
111,80,115,108,101,114,118,45;
141,144,146,66,157,216,88,159];

B = [75;
66;
57;
0;
1;
46;
14;
37];

C = 256*ones(64,1);

D = (A*B)./C;

fix(D)