/*
 * Copyright (c) 2022, RISE Research Institutes of Sweden AB.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDER AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 */


#ifndef EMTREES_H
#define EMTREES_H

#include <stdint.h>

typedef struct _EmlTreesNode {
    int8_t feature;
    float value;
    int16_t left;
    int16_t right;
} EmlTreesNode;


typedef struct _EmlTrees {
    int32_t n_nodes;
    EmlTreesNode *nodes;

    int32_t n_trees;
    int32_t *tree_roots;

    // int8_t n_features;
    // int8_t n_classes;
} EmlTrees;

typedef enum _EmlTreesError {
    EmlTreesOK = 0,
    EmlTreesUnknownError,
    EmlTreesInvalidClassPredicted,
    EmlTreesErrorLength,
} EmlTreesError;

const char *eml_trees_errors[EmlTreesErrorLength+1] = {
   "OK",
   "Unknown error",
   "Invalid class predicted",
   "Error length",
};

#ifndef EMTREES_MAX_CLASSES
#define EMTREES_MAX_CLASSES 10
#endif

static int32_t
eml_trees_predict_tree(const EmlTrees *forest, int32_t tree_root, const float *features, int8_t features_length) {
    int32_t node_idx = tree_root;

    // TODO: see if using a pointer node instead of indirect adressing using node_idx improves perf
    while (forest->nodes[node_idx].feature >= 0) {
        const int8_t feature = forest->nodes[node_idx].feature;
        const float value = features[feature];
        const float point = forest->nodes[node_idx].value;
        //printf("node %d feature %d. %d < %d\n", node_idx, feature, value, point);
        node_idx = (value < point) ? forest->nodes[node_idx].left : forest->nodes[node_idx].right;
    }
    return forest->nodes[node_idx].value;
}

int32_t
eml_trees_predict(const EmlTrees *forest, const float *features, int8_t features_length) {

    //printf("features %d\n", features_length);
    //printf("trees %d\n", forest->n_trees);
    //printf("nodes %d\n", forest->n_nodes);

    // FIXME: check if number of tree features is bigger than provided
    // FIXME: check if number of classes is bigger than MAX_CLASSES, error
 
    int32_t votes[EMTREES_MAX_CLASSES] = {0};
    for (int32_t i=0; i<forest->n_trees; i++) {
        const int32_t _class = eml_trees_predict_tree(forest, forest->tree_roots[i], features, features_length);
        //printf("pred[%d]: %d\n", i, _class);
        if (_class >= 0 && _class < EMTREES_MAX_CLASSES) {
            votes[_class] += 1;
        } else {
            return -EmlTreesInvalidClassPredicted;
        }
    }
    
    int32_t most_voted_class = -1;
    int32_t most_voted_votes = 0;
    for (int32_t i=0; i<EMTREES_MAX_CLASSES; i++) {
        //printf("votes[%d]: %d\n", i, votes[i]);
        if (votes[i] > most_voted_votes) {
            most_voted_class = i;
            most_voted_votes = votes[i];
        }
    }

    return most_voted_class;
}

#endif // EMTREES_H
