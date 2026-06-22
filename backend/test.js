// Import the functions you need from the SDKs you need
import { initializeApp } from "firebase/app";
// TODO: Add SDKs for Firebase products that you want to use
// https://firebase.google.com/docs/web/setup#available-libraries

// Your web app's Firebase configuration
const firebaseConfig = {
  apiKey: "AIzaSyBHGCL2VpywZv5QAY88b2PLIfniqRFKLXs",
  authDomain: "smarthouse-9925a.firebaseapp.com",
  projectId: "smarthouse-9925a",
  storageBucket: "smarthouse-9925a.firebasestorage.app",
  messagingSenderId: "514218841861",
  appId: "1:514218841861:web:c20f1a832531a927a6496c"
};

// Initialize Firebase
const app = initializeApp(firebaseConfig);

console.log(app)
