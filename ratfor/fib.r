# Fibonacci numbers in Ratfor
      program fib
      integer i, a, b, c
      a = 0
      b = 1
      for (i = 1; i .le. 10; i = i + 1) {
          write(*,10) i, a
10        format(1x,'fib(',i2,') = ',i6)
          c = a + b
          a = b
          b = c
      }
      end
