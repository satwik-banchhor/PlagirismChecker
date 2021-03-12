#include <stdio.h> //printf
#include <stdlib.h> //malloc
#include <string.h> //strtok strcat strcpy
#include <sys/types.h> //opendir
#include <dirent.h> //opendir
#include <time.h> //clock, CLOCKS_PER_SEC

#define MAXCHAR 1000000
#define CUTOFF 5 // Minimum Hit Score to consider a section is plagiarized

// Delimiter used for tokenizing lines of the files
char delim[] = " \t\r\n\v\f";

void plot() {
	FILE *gnuplotPipe = popen("gnuplot -persistent","w");
	fprintf(gnuplotPipe, "%s\n", "set term png");
    fprintf(gnuplotPipe, "%s\n", "set output \"time_comparison.png\"");
    fprintf(gnuplotPipe, "%s\n", "set xlabel \"Number of words in the Test File\"");
    fprintf(gnuplotPipe, "%s\n", "set ylabel \"Time Taken\"");
    fprintf(gnuplotPipe, "%s\n", "set title \"TIME COMPARISON PLOT\"");
    fprintf(gnuplotPipe, "%s\n", "plot \"data.dat\" using 1:2 title \"Time\", \"data.dat\" using 1:3 title \"Product of number of words in the 2 files (m * n)\" with lines");
    fflush(gnuplotPipe);
}

int get_index(char c) {
	int cap = c - 'A' + 1;
	int small = c - 'a' + 1;
	int num = c - '0' + 1;
	if (cap >= 1 && cap <= 26) return cap;
	if (small >= 1 && small <= 26) return small;
	if (num >= 1 && num <= 10) return 26 + num;
	return 0;
}

// Hashing constants
const int mod = 1e9 + 9;
const int base = 37;

// Hash Function
int hash(char *token) {
	int len = strlen(token);
	long hash_value = 0;
	for (int i = 0; i < len; ++i) {
		int index = get_index(token[i]);
		if (index) {
			hash_value = ((hash_value * base) % mod + index) % mod;
		}
	}
	return hash_value;
}

int is_break(char *line) {
	return (line[0] == '<' && line[1] == 'b' && line[2] == 'r' && line[3] == '>');
}

int get_file(int **text, char **addr, int **word_lengths) {
	char *line = malloc(100000 * sizeof(char));
	*text = malloc(5000 * sizeof(int));
	*word_lengths = malloc(5000 * sizeof(int));

	// Opening the file
	FILE *in_file = fopen(*addr, "r");

	if (in_file == NULL) {
		fprintf(stderr, "Could not open file %s", *addr);
		exit(1);
	}

	int word_index = 0;

	while (fgets(line, MAXCHAR, in_file) != NULL) {
		// Reading the file line by line
		if (is_break(line)) break;
		char *token = strtok(line, delim);
		while (token != NULL) {
			// hashing each word (tokens separaed by spaces in the file)
			(*text)[++word_index] = hash(token);
			(*word_lengths)[word_index] = strlen(token);
			token = strtok(NULL, delim);
		}
	}
	
	// Closing the in_file
	fclose(in_file);
	return word_index;
}

int max(int a, int b, int c, int d) {
	if (a >= b && a >= c && a >= d) return a;
	if (b >= a && b >= c && b >= d) return b;
	if (c >= b && c >= a && c >= d) return c;
	if (d >= b && d >= c && d >= a) return d;
}

struct triplet_smo {
	int s, m, o;
};

int compare (const void * a, const void * b) {
	struct triplet_smo data1 = *(struct triplet_smo *)a, data2 = *(struct triplet_smo *)b;
	if (data1.s < data2.s) return -1;
	if (data1.s == data2.s) {
		if (data1.m < data2.m) return -1;
		if (data1.m == data2.m)
			if (data1.o < data2.o) return -1;
	}
	return 1;
}

float find_similarity(int **input, int **word_lengths, int **test, int n, int m) {
	int *t1 = *input, *t2 = *test;
	if (!n || !m) return 0.0;
	// fprintf(stderr, "%d %d\n", n, m);
	int S[2][m + 1], M[2][m + 1], Oi[2][m + 1], plagiarized_sections[n + 1];
	memset(plagiarized_sections, 0, sizeof(plagiarized_sections)); // Initialize plagiarized_sections with 0s
	memset(S, 0, sizeof(S)); // Initialize Score matrix S[i][j] with 0s
	memset(M, 0, sizeof(M)); // Initialize Mazimum previous matrix M[i][j] with 0s
	memset(Oi, 0, sizeof(Oi)); // Initialize Origin matrix Oi[i][j] with 0s
	for (int i = 0; i < n; ++i) {
		int max_similarity_score_i = 0;
		int similarity_origin_i = i + 1;
		for (int j = 1; j <= m; ++j) {
			// Updating the matrices S, M & Oi
			if (t1[i + 1] == t2[j]) {
				S[i & 1 ^ 1][j] = S[i & 1][j - 1] + 1;
				M[i & 1 ^ 1][j] = max(0, 0, S[i & 1][j - 1], M[i & 1][j - 1]);
				Oi[i & 1 ^ 1][j] = Oi[i & 1][j - 1];
			}
			else {
				struct triplet_smo a1, a2, a3, arr[3];
				a1.s = S[i & 1][j];a1.m = M[i & 1][j];a1.o = Oi[i & 1][j];
				a2.s = S[i & 1 ^ 1][j - 1];a2.m = M[i & 1 ^ 1][j - 1];a2.o = Oi[i & 1 ^ 1][j - 1];
				a3.s = S[i & 1][j - 1];a3.m = M[i & 1][j - 1];a3.o = Oi[i & 1][j - 1];
				arr[0] = a1; arr[1] = a2; arr[2] = a3;
				qsort(arr, 3, sizeof(struct triplet_smo), compare);
				S[i & 1 ^ 1][j] = max(0, 0, 0, arr[2].s - 1);
				M[i & 1 ^ 1][j] = max(0, 0, arr[2].m, arr[2].s);
				Oi[i & 1 ^ 1][j] = arr[2].o;
				if (M[i & 1 ^ 1][j] - S[i & 1 ^ 1][j] >= CUTOFF) S[i & 1 ^ 1][j] = 0;
				if (S[i & 1 ^ 1][j] == 0) {
					M[i & 1 ^ 1][j] = 0;
					Oi[i & 1 ^ 1][j] = i + 1;
				}
			}

			// Updating similarities for this i + 1
			/* Max similarity score gets updated by a 
			 score S[i][j] if it atleast CUTOFF and isn't predominated */
			if (M[i & 1 ^ 1][j] < S[i & 1 ^ 1][j] && max_similarity_score_i < S[i & 1 ^ 1][j]) {
				max_similarity_score_i = S[i & 1 ^ 1][j];
				similarity_origin_i = Oi[i & 1 ^ 1][j];
			}
			else if (max_similarity_score_i == S[i & 1 ^ 1][j]) {
				similarity_origin_i = max(0, 0, similarity_origin_i, Oi[i & 1 ^ 1][j]);
			}

			// UNCOMMENT: Next 3 commenets to see the Score matrix
			// if (S[i & 1 ^ 1][j]) fprintf(stderr, "%d ", S[i & 1 ^ 1][j]);
			// else fprintf(stderr, "  ");
		}
		// fprintf(stderr, "\n");

		// Marking a plagiarized section
		if (max_similarity_score_i >= CUTOFF) {
			++plagiarized_sections[similarity_origin_i + 1];
			if (i < n - 1) --plagiarized_sections[i + 2];
		}
	}

	int plagiarized_total = 0; // Count of plagiarized words
	int total = 0;
	// Taking prefix sum to find which sections of the text t1 are plagiarized
	for (int i = 1; i <= n; ++i) {
		plagiarized_sections[i] += plagiarized_sections[i - 1];
		if (plagiarized_sections[i] > 0) 
			plagiarized_total += (*word_lengths)[i];
		total += (*word_lengths)[i];
	}

	return 100 * (plagiarized_total/((float)(total)));
}

int main(int argc, char* argv[]) {
	// Collecting  data for time taken
	FILE* data = fopen("data.dat", "w");
	clock_t start, end;
	double time;

	int *input_text, *comparison_text, *word_lengths, *word_lengths_test;

	int num_words_input = get_file(&input_text, &argv[1], &word_lengths);
	// fprintf(stderr, "Number of words in input file are: %d\n", num_words);

	//Opening Directory
	DIR* corpus_files_dir = opendir(argv[2]);
    
	if (corpus_files_dir == NULL) {
		fprintf(stderr, "Could not open corpus files' directory directory%s\n", argv[2]);
		return 1;
	}

	struct dirent* corpus_dir_entry;
	char *entry_file_name = malloc(1000 * sizeof(char));

	while ((corpus_dir_entry = readdir(corpus_files_dir)) != NULL) {
		start = clock();
		// Iterating throught all the entries of the corpus_files directory
		strcpy(entry_file_name, argv[2]);
		strcat(entry_file_name, corpus_dir_entry->d_name);
		int l = strlen(entry_file_name);
		if (!(entry_file_name[l - 3] == 't' && entry_file_name[l - 2] == 'x' && entry_file_name[l - 3] == 't')) continue;
		// Opening all .txt files
		int num_words = get_file(&comparison_text, &entry_file_name, &word_lengths_test);

		// ------ CHECK SIMILARITY ------
		fprintf(stderr, "-------------------------------------\n");
		fprintf(stderr, "Similarity between the input file and the file %s is: %f %%. \n", corpus_dir_entry->d_name, find_similarity(&input_text, &word_lengths, &comparison_text, num_words_input, num_words));
		// fprintf(stderr, "Number of words in %s are: %d\n", corpus_dir_entry->d_name, num_words);	
		end = clock();
		time = ((double)(end - start))/ CLOCKS_PER_SEC;
		fprintf(data,"%d\t%f\t%f\n", num_words, time, (((double)(num_words_input*num_words))/10000000));
	}

	// GNU Plot for Time Comparison
	plot();

    return 0;
}
