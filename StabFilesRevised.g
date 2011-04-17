FullGroup := function(GeneratorsList,VarOrderList)
        local g;

        g := Group(GeneratorsList);

        return Elements(g);
end;

StabChainGap := function(GeneratorsList, VarOrderList)
        local g, StabRecord, NewGeneratorsList;
        
        g := Group(GeneratorsList);
        StabRecord := StabChain(g,VarOrderList);
        NewGeneratorsList := StabRecord.generators;

        return NewGeneratorsList;
end;


StabChainReduced := function(GeneratorsList, VarOrderList)
        local g, NewGeneratorsList, ElemList, NewGeneratorsListDup, bool, i, index, j, RepElem, MappedTo, FinalGens, gen;

        NewGeneratorsListDup := [];
        g := Group(GeneratorsList);
        i := 1;
        index := VarOrderList[i];
        bool := true;

        while bool=true do
            ElemList := Orbit(g,index);
            Print("Orbit:", ElemList, "\n");
            for j in ElemList do
                if j <> index then
                  RepElem := RepresentativeAction(g,index,j);
                  Print("OrbitRep:", RepElem, "\n");
                  Add(NewGeneratorsListDup, [ j, RepElem]);
                fi;
            od;
            g:= Stabilizer(g,index);
            Print("Stabilizer:", g, "\n");
            i:= i+1;
            index := VarOrderList[i];
            if g = Group(()) 
                then bool:= false; 
            fi;
        od;    
        
        MappedTo := [];
        FinalGens := [];
        Print(NewGeneratorsListDup);
        for gen in NewGeneratorsListDup do
            if not(gen[1] in MappedTo) then
                Add(MappedTo, gen[1]);
                Add(FinalGens, gen[2]);
            fi;
        od;
        
        NewGeneratorsList := DuplicateFreeList(FinalGens);
        return NewGeneratorsList;
end;

StabChainME := function(GeneratorsList, VarOrderList)
        local g, NewGeneratorsList, ElemList, NewGeneratorsListDup, bool, i, index, j, RepElem, gen;

        NewGeneratorsListDup := [];
        g := Group(GeneratorsList);
        i := 1;
        index := VarOrderList[i];
        bool := true;

        while bool=true do
            ElemList := Orbit(g,index);
            Print("Orbit:", ElemList, "\n");
            for j in ElemList do
                  RepElem := RepresentativeAction(g,index,j);
                  Print("OrbitRep:", RepElem, "\n");
                  Add(NewGeneratorsListDup, RepElem);
            od;
            g:= Stabilizer(g,index);
            Print("Stabilizer:", g, "\n");
            i:= i+1;
            index := VarOrderList[i];
            if g = Group(()) 
                then bool:= false; 
            fi;
        od;    
        
        NewGeneratorsList := DuplicateFreeList(NewGeneratorsListDup);
        return NewGeneratorsList;
end;

RandomElem := function(n, GeneratorsList)
        local NewGeneratorsList, NewGeneratorsListDup, i, g, RepElem;

        NewGeneratorsListDup := [];
        g := Group(GeneratorsList);

        for i in [1..n] do
            RepElem := Random(g);
            Add(NewGeneratorsListDup, RepElem);
        od;

        NewGeneratorsList := DuplicateFreeList(NewGeneratorsListDup);
        return NewGeneratorsList;
end;

RandomGens := function(GeneratorsList)
    local G,H,HGens,perm;
    G := Group(GeneratorsList);
    HGens := [()];
    H := Group(HGens);
    while Size(G) <> Size(H) do
        perm := Random(G);
        while (perm in H) do
            perm := Random(G);
        od;
        Add(HGens, perm);
        H := Group(HGens);
    od;
    return HGens;
end;

