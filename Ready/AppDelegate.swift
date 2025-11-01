/*
 AppDelegate.swift
 Copyright (C) 2019 Dieter Baron
 
 This file is part of Ready, a home computer emulator for iPad.
 The authors can be contacted at <ready@tpau.group>.
 
 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions
 are met:
 
 1. Redistributions of source code must retain the above copyright
 notice, this list of conditions and the following disclaimer.
 
 2. The names of the authors may not be used to endorse or promote
 products derived from this software without specific prior
 written permission.
 
 THIS SOFTWARE IS PROVIDED BY THE AUTHORS ``AS IS'' AND ANY EXPRESS
 OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY
 DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
 IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

import UIKit
import CoreData
import Emulator

@UIApplicationMain
class AppDelegate: UIResponder, UIApplicationDelegate {

    var window: UIWindow?


    func application(_ application: UIApplication, didFinishLaunchingWithOptions launchOptions: [UIApplication.LaunchOptionsKey: Any]?) -> Bool {
        // Override point for customization after application launch.

        return true
    }

    func applicationWillResignActive(_ application: UIApplication) {
        // Sent when the application is about to move from active to inactive state. This can occur for certain types of temporary interruptions (such as an incoming phone call or SMS message) or when the user quits the application and it begins the transition to the background state.
        // Use this method to pause ongoing tasks, disable timers, and invalidate graphics rendering callbacks. Games should use this method to pause the game.
        // TODO: pause emulation
    }

    func applicationDidEnterBackground(_ application: UIApplication) {
        // Use this method to release shared resources, save user data, invalidate timers, and store enough application state information to restore your application to its current state in case it is terminated later.
        // If your application supports background execution, this method is called instead of applicationWillTerminate: when the user quits.
        // TODO: save emulation state
    }

    func applicationWillEnterForeground(_ application: UIApplication) {
        // Called as part of the transition from the background to the active state; here you can undo many of the changes made on entering the background.
    }

    func applicationDidBecomeActive(_ application: UIApplication) {
        // Restart any tasks that were paused (or not yet started) while the application was inactive. If the application was previously in the background, optionally refresh the user interface.
        updateLibrary()
    }

    func applicationWillTerminate(_ application: UIApplication) {
        // Called when the application is about to terminate. Save data if appropriate. See also applicationDidEnterBackground:.
        // Saves changes in the application's managed object context before the application terminates.
        self.saveContext()
    }
    
    func application(_ app: UIApplication, open url: URL, options: [UIApplication.OpenURLOptionsKey : Any] = [:]) -> Bool {
        do {
            let directoryURL = Defaults.inboxURL
            try ensureDirectory(directoryURL)
            
            // TODO: check that media item can be created from file
            
            _ = url.startAccessingSecurityScopedResource()
            
            let fileCoordinator = NSFileCoordinator(filePresenter: nil)

            var error: NSError?
            
            var ok = false
            
            fileCoordinator.coordinate(readingItemAt: url, options: .withoutChanges, error: &error) { coordinatedUrl in
                do {
                    let fileURL = try uniqueName(directory: directoryURL, name: url.lastPathComponent, pathExtension: url.pathExtension)
                    try FileManager.default.copyItem(at: coordinatedUrl, to: fileURL)
                    ok = true
                }
                catch {
                    print("can't open \(url): \(error.localizedDescription)")
                }
            }
            url.stopAccessingSecurityScopedResource()

            if !ok, let error = error {
                print("can't open \(url): \(error.localizedDescription)")
            }
            
            if ok /* TODO: && viceThread == nil */ {
                if let tabBarController = window?.rootViewController as? TabBarViewController {
                    tabBarController.mySelectedTab = .inbox
                }
            }
            
            return ok
        }
        catch {
            url.stopAccessingSecurityScopedResource()
            print("can't open \(url): \(error.localizedDescription)")
            return false
        }
    }
    
    func application(_ application: UIApplication, shouldRestoreApplicationState coder: NSCoder) -> Bool {
        return true
    }
    
    func application(_ application: UIApplication, shouldSaveApplicationState coder: NSCoder) -> Bool {
        return true
    }
    
    // MARK: - Globals
    

    // MARK: - Core Data stack

    lazy var persistentContainer: NSPersistentContainer = {
        /*
         The persistent container for the application. This implementation
         creates and returns a container, having loaded the store for the
         application to it. This property is optional since there are legitimate
         error conditions that could cause the creation of the store to fail.
        */
        let container = NSPersistentContainer(name: "C64")
        container.loadPersistentStores(completionHandler: { (storeDescription, error) in
            if let error = error as NSError? {
                // Replace this implementation with code to handle the error appropriately.
                // fatalError() causes the application to generate a crash log and terminate. You should not use this function in a shipping application, although it may be useful during development.
                 
                /*
                 Typical reasons for an error here include:
                 * The parent directory does not exist, cannot be created, or disallows writing.
                 * The persistent store is not accessible, due to permissions or data protection when the device is locked.
                 * The device is out of space.
                 * The store could not be migrated to the current model version.
                 Check the error message to determine what the actual problem was.
                 */
                fatalError("Unresolved error \(error), \(error.userInfo)")
            }
        })
        return container
    }()

    // MARK: - Core Data Saving support

    func saveContext () {
        let context = persistentContainer.viewContext
        if context.hasChanges {
            do {
                try context.save()
            } catch {
                // Replace this implementation with code to handle the error appropriately.
                // fatalError() causes the application to generate a crash log and terminate. You should not use this function in a shipping application, although it may be useful during development.
                let nserror = error as NSError
                fatalError("Unresolved error \(nserror), \(nserror.userInfo)")
            }
        }
    }

    
    @objc func mergeContextChanges(_ notification: Notification) {
        persistentContainer.viewContext.mergeChanges(fromContextDidSave: notification)
    }

    // MARK: - Utility functions
    
    private var isUpdateLibraryRunning = false
    func updateLibrary() {
        guard !isUpdateLibraryRunning else { return }
        isUpdateLibraryRunning = true
        let context = persistentContainer.newBackgroundContext()
        NotificationCenter.default.addObserver(self, selector: #selector(mergeContextChanges(_:)), name: NSNotification.Name.NSManagedObjectContextDidSave, object: context)

        context.perform {
            self.importDocumentsDirectory(context: context)
            self.deleteExpiredGames(context: context)

            DispatchQueue.main.async {
                NotificationCenter.default.removeObserver(self, name: NSNotification.Name.NSManagedObjectContextDidSave, object: context)
                self.isUpdateLibraryRunning = false
            }
        }
    }
    
    func importDocumentsDirectory(context: NSManagedObjectContext) {
        do {
            let libraryURL = Defaults.libraryURL
            let documentURL = Defaults.documentURL
            let fileManager = FileManager.default
            
            let ignoredDirectories: Set<String> = [
                "Inbox",
                Defaults.exporedName,
                Defaults.inboxName
            ]
            
            for directory in try fileManager.contentsOfDirectory(at: documentURL, includingPropertiesForKeys: [.isDirectoryKey], options: [.skipsPackageDescendants, .skipsHiddenFiles]) {
                let isDirectory = try directory.resourceValues(forKeys: [.isDirectoryKey]).isDirectory ?? true
                let name = directory.lastPathComponent
                if !isDirectory || ignoredDirectories.contains(name) {
                    continue
                }
                print("importing \(name)")
                let targetUrl = try uniqueName(directory: libraryURL, name: name, pathExtension: "")
                do {
                    try fileManager.moveItem(at: directory, to: targetUrl)
                    _ = Game(name: name, directory: targetUrl.lastPathComponent, insertInto: context, importFiles: true)
                }
                catch { }
            }
            try context.save()
        }
        catch { }
    }
    
    func deleteExpiredGames(context: NSManagedObjectContext) {
        do {
            var directories = [String]()
            
            let fetchRequest = NSFetchRequest<NSFetchRequestResult>(entityName: "Game")
            let cutoffDate = Date(timeIntervalSinceNow: -30 * 24 * 60 * 60)
            fetchRequest.predicate = NSPredicate(format: "deletedDate < %@", argumentArray: [ cutoffDate ])

            guard let results = try context.fetch(fetchRequest) as? [Game] else { return }

            for game in results {
                print("deleting game \(game.name)")
                directories.append(game.directory)
                context.delete(game)
            }
            
            try context.save()
            
            let fileManager = FileManager.default
            let libraryUrl = Defaults.libraryURL
            
            for directory in directories {
                do {
                    print("deleting directory \(directory)")
                    try fileManager.removeItem(at: libraryUrl.appendingPathComponent(directory))
                }
                catch { }
            }
        }
        catch { }
    }
}
