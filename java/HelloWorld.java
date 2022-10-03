class HelloWorld {
    public static void main(String[] args) {
        megaFoo("100", 100, true);
        superFoo();
        System.out.println("Hello, World!"); 
    }

    static void megaFoo(String inStr, Integer inInt, Boolean inBool) {
        System.out.printf("Data: %s, %d, %b\n", inStr, inInt, inBool); 
    }

    static void superFoo() {
        System.out.println("superFoo"); 
    }
}
