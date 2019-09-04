
# How to run the program

1. User Docker `docker-compose up -d` to start enviroment
2. Exec a bash shell on the enviorment `docker-compose exec gcc bash`
3. Do all the stuff you want, such as
   - `./cruncher dataset-sf100 queries-test.csv out.csv`
   - `diff out.csv queries-test-output-sf100.csv` in linux host
   - `diff --strip-trailing-cr out.csv queries-test-output-sf100.csv` in windows host and files container \cr

# Profiling

1. Compile with -pg
2. Run program (now gmon.out should exist)
   - Use the profiling version of cruncher: `./cruncher-profile dataset-sf100 queries-test.csv out.csv`
3. Execute `gprof cruncher-profile gmon.out > analysis.txt`
4. Anaylze 🤓
