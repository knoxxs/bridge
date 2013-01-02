		
import java.io.*;
import java.net.*;

public class client {

    public static void main(String[] args) throws IOException {

        Socket kkSocket = null;
        PrintWriter out = null;
        BufferedReader in = null;

        try {
            kkSocket = new Socket("localhost", 4444);
            out = new PrintWriter(kkSocket.getOutputStream(), true);
            in = new BufferedReader(new InputStreamReader(kkSocket.getInputStream()));
        } catch (UnknownHostException e) {
            System.err.println("Don't know about host: taranis.");
            System.exit(1);
        } catch (IOException e) {
            System.err.println("Couldn't get I/O for the connection to: taranis.");
            System.exit(1);
        }

        BufferedReader stdIn = new BufferedReader(new InputStreamReader(System.in));
        String fromServer;
        String fromUser;

        out.write('a');//choice
        out.write("11111000\n",0,9);  //need to add \n not \0 as on client side using sscnf not strcpy
        out.write("abcd\n",0,5);
        out.write("0123456789\n",0,11);
        out.write("check\0",0,6);

       //out.println("sdgfdg");
        
       /* while ((fromServer = in.readLine()) != null) {
            System.out.println("Server: " + fromServer);
            if (fromServer.equals("Bye.")) {
                break;
            }

            fromUser = stdIn.readLine();
            if (fromUser != null) {
                System.out.println("Client: " + fromUser);
                out.println(fromUser);
            }
        }
        */
        int i;
        for(i=0 ; i < 1000000; i++){

        }
        // try{
        //     //do what you want to do before sleeping
        //     Thread.currentThread().sleep(10000000);//sleep for 1000 ms
        //     //do what you want to do after sleeptig
        // }
        //     catch(InterruptedException ie){
        //     //If this thread was intrrupted by nother thread 
        // }

        out.close();
        in.close();
        stdIn.close();
        kkSocket.close();
    }
}
