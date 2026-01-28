#include <vector>
#include <algorithm>
#include <string>

std::vector<std::string> buildTabs(int numRestaurantes){
    std::vector<std::string> spacesForRestaurantes(numRestaurantes);

    spacesForRestaurantes[0] = "";

    for (int i = 1; i < numRestaurantes; i++) {
        spacesForRestaurantes[i] = spacesForRestaurantes[i - 1] + "\t\t\t\t";
    }

    return spacesForRestaurantes;
}

std::string buildTitulo(int numRestaurantes){
    std::string res = "RESTAURANTE";
    std::string tabs = "\t\t\t";
    std::string stringFinal = "";
    for (int i = 1; i < numRestaurantes+1; i++){
        stringFinal.append(res + " " + std::to_string(i) + ":" + tabs);
    }
    stringFinal.append("\n");
    return stringFinal;
}

// "RESTAURANTE 1:\t\t\tRESTAURANTE 2:\t\t\tRESTAURANTE 3:\t\t\tRESTAURANTE 4:\n"