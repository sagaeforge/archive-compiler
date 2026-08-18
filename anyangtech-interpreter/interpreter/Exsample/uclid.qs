
def f()
    for b <- 2 to 1000 step 1
        if( b == 2 or b == 3 or b == 5 or b == 7 )
            print b, " "
        end
        if not(b % 2 == 0 or b % 3 == 0 or b % 5 == 0 or b % 7 == 0)
            print b, " "
        end
    end
end

f()
