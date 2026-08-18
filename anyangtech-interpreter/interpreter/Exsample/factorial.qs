def factorial(n)
    if n <= 2
        return 2
    end
    return n * factorial(n - 1)
end

println factorial(5)
