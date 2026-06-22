import { db } from "./src/firebase.js"

async function seed() {

    await db.collection("users").doc("testUser").set({
        id: "1",
        email: "test@test.com",
        passwordHash: "test"
    });

    await db.collection("users").doc("testUser2").set({
        id: "2",
        email: "test2@test.com",
        passwordHash: "test2"
    });

    console.log("Seed complete");
}

seed();
