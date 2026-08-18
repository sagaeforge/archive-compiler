

def h(no, x, y)
    if(no > 1)
        h(no - 1, x, 6 - x - y)
    end
    println "원반 [", no, "] 를 ", x, "기둥에서 ", y, " 기둥으로 옮김"
    if(no > 1)
        h(no - 1, 6 - x - y, y)
    end
end

h(3, 1, 3)