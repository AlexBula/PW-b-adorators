#include "blimit.hpp"

// unsigned int bvalue(unsigned int method, unsigned long node_id) {
//     switch (method) {
//     default: return node_id % 42;
//     case 0: return 4;
//     case 1: return 7;
//     }        
// }

// testy z fb
// unsigned int bvalue(unsigned int method, unsigned long node_id) {
//     return method;
// }

// unsigned int bvalue(unsigned int method, unsigned long node_id) {

//     switch (method) {

//     case 0: return 1;

//     default: switch (node_id) {

//         case 0: return 2;

//         case 1: return 2;

//         default: return 1;

//         }

//     }

// }

// road
unsigned int bvalue(unsigned int method, unsigned long node_id) {
    switch (method) {
    default: return (2 * node_id + method) % 10;
    case 0: return 4;
    case 1: return 7;
    } 
}

// youtube
// unsigned int bvalue(unsigned int method, unsigned long node_id) {
//     return 20;
// }

// // fb
// unsigned int bvalue(unsigned int method, unsigned long node_id) {
//     return 3;
// }


// unsigned int bvalue(unsigned int method, unsigned long node_id) {
//     switch (method) {
//     case 0: return 1;
//     case 1: return 42;
//     case 2: {
//         if(node_id%3==0){return 1;}
//         if(node_id%3==1){return 22;}
//         if(node_id%3==2){return 38;}
//         }
//     }
// }

// paper
// unsigned int bvalue(unsigned int method, unsigned long node_id) {
//     switch (method) {
//     case 0: return 1;
//     default: switch (node_id) {
//         case 0: return 2;
//         case 1: return 2;
//         default: return 1;
//         }
//     }
// }
