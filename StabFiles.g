GeneratorsBasic := function(GeneratorsList)

        return GeneratorsList;
end;

FullGroup := function(GeneratorsList)
        local g;

        g := Group(GeneratorsList);

        return Elements(g);
end;

StabChainGap := function(GeneratorsList)
        local g, StabRecord, NewGeneratorsList;
        
        g := Group(GeneratorsList);
        StabRecord := StabChain(g);
        NewGeneratorsList := StabRecord.generators;

        return NewGeneratorsList;
end;

StabComplete := function(GeneratorsList)
        local g, NewGeneratorsList, n, Elem, i, NewGeneratorsListDup;

        NewGeneratorsListDup := [];
        g := Group(GeneratorsList);
        n := LargestMovedPoint(g);
        
        for i in [1..n] do
            Elem := Representative(Stabilizer(g,i));
            Add(NewGeneratorsListDup, Elem);
        od;
        
        NewGeneratorsList := DuplicateFreeList(NewGeneratorsListDup);
        return NewGeneratorsList;
end;

StabCompleteN := function(n, GeneratorsList)
        local g, NewGeneratorsList, Elem, i, NewGeneratorsListDup;

        NewGeneratorsListDup := [];
        g := Group(GeneratorsList);
        
        for i in [1..n] do
            Elem := Representative(Stabilizer(g,i));
            Add(NewGeneratorsListDup, Elem);
        od;
        
        NewGeneratorsList := DuplicateFreeList(NewGeneratorsListDup);
        return NewGeneratorsList;
end;

OrbitComplete := function(GeneratorsList)
        local g, NewGeneratorsList, n, ElemList, NewGeneratorsListDup, RepElem, i,j;

        NewGeneratorsListDup := [];
        g := Group(GeneratorsList);
        n := LargestMovedPoint(g);

        for i in [1..n] do
            ElemList := Orbit(g,i);
            for j in ElemList do
                RepElem := RepresentativeAction(g,i,j);
                Add(NewGeneratorsListDup, RepElem);
            od;
        od;

        NewGeneratorsList := DuplicateFreeList(NewGeneratorsListDup);
        return NewGeneratorsList;
end;

OrbitCompleteN := function(n, GeneratorsList)
        local g, NewGeneratorsList, ElemList, NewGeneratorsListDup, RepElem, i, j;

        NewGeneratorsListDup := [];
        g := Group(GeneratorsList);

        for i in [1..n] do
                Print("i ", i, "\n");
            ElemList := Orbit(g,i);
            for j in ElemList do
                RepElem := RepresentativeAction(g,i,j);
                Print("RepElem", RepElem, "\n");
                Add(NewGeneratorsListDup, RepElem);
            od;
        od;

        NewGeneratorsList := DuplicateFreeList(NewGeneratorsListDup);
        return NewGeneratorsList;
end;

StabChainME := function(GeneratorsList)
        local g, NewGeneratorsList, ElemList, NewGeneratorsListDup, bool, i, j, RepElem;

        NewGeneratorsListDup := [];
        g := Group(GeneratorsList);
        i := 1;
        bool := true;

        while bool=true do
            ElemList := Orbit(g,i);
            Print("Orbit:", ElemList, "\n");
            for j in ElemList do
                RepElem := RepresentativeAction(g,i,j);
                Print("OrbitRep:", RepElem, "\n");
                Add(NewGeneratorsListDup, RepElem);
            od;
            g:= Stabilizer(g,i);
            Print("Stabilizer:", g, "\n");
            i:= i+1;
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

